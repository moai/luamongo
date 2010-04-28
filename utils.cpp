#include <iostream>
#include <client/dbclient.h>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#if !defined(LUA_VERSION_NUM) || (LUA_VERSION_NUM < 501)
#include <compat-5.1.h>
#endif
};

#include "utils.h"
#include "common.h"

using namespace mongo;

static void lua_push_value(lua_State *L, const BSONElement &elem);

static void bson_to_array(lua_State *L, const BSONObj &obj) { 
    BSONObjIterator it = BSONObjIterator(obj);
    
    lua_newtable(L);

    int n = 1;
    while (it.more()) {
        BSONElement elem = it.next();

        lua_push_value(L, elem);
        lua_rawseti(L, -2, n++);
    } 
}

static void bson_to_table(lua_State *L, const BSONObj &obj) {
    BSONObjIterator it = BSONObjIterator(obj);

    lua_newtable(L);

    while (it.more()) {
        BSONElement elem = it.next();
        const char *key = elem.fieldName();

        lua_pushstring(L, key);
        lua_push_value(L, elem);
        lua_rawset(L, -3);
    }
}

static void lua_push_value(lua_State *L, const BSONElement &elem) {
    int type = elem.type();

    switch(elem.type()) {
    case mongo::Undefined:
    case mongo::jstNULL:
        lua_pushnil(L);
        break;
    case mongo::NumberInt:
        lua_pushinteger(L, elem.number());
        break;
    case mongo::NumberDouble:
        lua_pushnumber(L, elem.number());
        break;
    case mongo::Bool:
        lua_pushboolean(L, elem.boolean());
        break;
    case mongo::String:
        lua_pushstring(L, elem.valuestr()); 
        break;
    case mongo::Array:
        bson_to_array(L, elem.embeddedObject());
        break;
    case mongo::Object:
        bson_to_table(L, elem.embeddedObject());
        break;
    case mongo::jstOID:
        lua_pushstring(L, elem.__oid().str().c_str());
        break;
    case mongo::EOO:
        break;
    default:
        luaL_error(L, LUAMONGO_UNSUPPORTED_BSON_TYPE);
    }
}

static void lua_append_bson(lua_State *L, const char *key, int stackpos, BSONObjBuilder *builder) {
    int type = lua_type(L, stackpos);

    if (type == LUA_TTABLE) {
        BSONObjBuilder *b = new BSONObjBuilder();

        lua_pushnil(L); 
        while (lua_next(L, stackpos-1) != 0) {
            const char *k = lua_tostring(L, -2);
            lua_append_bson(L, k, -1, b);
            lua_pop(L, 1);
        }

        builder->append(key, b->done());
    } else if (type == LUA_TNIL) {
        builder->appendNull(key);
    } else if (type == LUA_TNUMBER) {
        builder->append(key, lua_tonumber(L, stackpos));
    } else if (type == LUA_TBOOLEAN) {
        builder->appendBool(key, lua_toboolean(L, stackpos));
    } else if (type == LUA_TSTRING) {
        builder->append(key, lua_tostring(L, stackpos));
    } else {
        luaL_error(L, LUAMONGO_UNSUPPORTED_LUA_TYPE, luaL_typename(L, stackpos));
    }
}

void bson_to_lua(lua_State *L, const BSONObj &obj) {
    if (obj.isEmpty()) {
        lua_pushnil(L);
    } else {
        bson_to_table(L, obj);
    }
}

void lua_to_bson(lua_State *L, int stackpos, BSONObj &obj) {
    BSONObjBuilder *builder = new BSONObjBuilder();

    lua_pushnil(L);
    while (lua_next(L, stackpos) != 0) {
        const char *k = lua_tostring(L, -2);
        lua_append_bson(L, k, -1, builder);
        lua_pop(L, 1);
    }

    obj = builder->done();
}
