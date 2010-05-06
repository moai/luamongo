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

#include "common.h"

using namespace mongo;

// TODO:
//    all of this should be in Lua so it can get JIT wins
//    bind the bson typeids
//    each type could have its cached metatable (from registry)

// all these types are represented as tables
// the metatable entry __bsontype dictates the type
// the t[1] represents the object itself, with some types using other fields

namespace {
// pushes onto the stack a new table with the bsontype metatable set up
void push_bsontype_table(lua_State* L, mongo::BSONType bsontype)
{
    lua_newtable(L);
    lua_newtable(L);
    lua_pushstring(L, "__bsontype");
    lua_pushinteger(L, bsontype);
    lua_settable(L, -3);
    lua_setmetatable(L, -2);
}
} // anonynous namespace


static int bson_type_Date(lua_State *L) {
    push_bsontype_table(L, mongo::Date);
    lua_pushvalue(L, 1);
    lua_rawseti(L, -2, 1); // t[1] = function arg #1
    return 1;
}

static int bson_type_Timestamp(lua_State*L) {
    push_bsontype_table(L, mongo::Timestamp);
    // no arg
    return 1;
}

static int bson_type_RegEx(lua_State *L) {
    push_bsontype_table(L, mongo::RegEx);
    lua_pushvalue(L, 1);
    lua_rawseti(L, -2, 1); // t[1] = function arg #1
    lua_pushvalue(L, 2);
    lua_rawseti(L, -2, 2); // set t[2] = function arg #2
    return 1;
}

static int bson_type_NumberInt(lua_State *L) {
    push_bsontype_table(L, mongo::NumberInt);
    lua_pushvalue(L, 1);
    lua_rawseti(L, -2, 1); // t[1] = function arg #1
    return 1;
}

static int bson_type_Symbol(lua_State *L) {
    push_bsontype_table(L, mongo::Symbol);
    lua_pushvalue(L, 1);
    lua_rawseti(L, -2, 1); // t[1] = function arg #1
    return 1;
}

int mongo_bsontypes_register(lua_State *L) {
    static const luaL_Reg bsontype_methods[] = {
        {"Date", bson_type_Date},
        {"Timestamp", bson_type_Timestamp},
        {"RegEx", bson_type_RegEx},
        {"NumberInt", bson_type_NumberInt},
        {"Symbol", bson_type_Date},
        {NULL, NULL}
    };

    luaL_register(L, LUAMONGO_ROOT, bsontype_methods);

    return 1;
}

