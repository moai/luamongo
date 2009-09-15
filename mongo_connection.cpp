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

extern int cursor_create(lua_State *L, DBClientConnection *connection, const char *ns, const Query &query);

extern void lua_to_bson(lua_State *L, int stackpos, BSONObj &obj);
extern void bson_to_lua(lua_State *L, const BSONObj &obj);

static int connection_new(lua_State *L) {
    int resultcount = 1;

    try {
	DBClientConnection **connection = (DBClientConnection **)lua_newuserdata(L, sizeof(DBClientConnection *));
        *connection = new DBClientConnection();

        luaL_getmetatable(L, LUAMONGO_CONNECTION);
        lua_setmetatable(L, -2);
    } catch (std::exception &e) {
        lua_pushnil(L);
	lua_pushfstring(L, LUAMONGO_ERR_CONNECTION_FAILED, e.what());
	resultcount = 2;
    }

    return resultcount;
}

static int connection_connect(lua_State *L) {
    void *ud = 0;

    ud = luaL_checkudata(L, 1, LUAMONGO_CONNECTION);
    DBClientConnection *connection = *((DBClientConnection **)ud); 

    const char *connectstr = luaL_checkstring(L, 2);

    try {
	connection->connect(connectstr);
    } catch (std::exception &e) {
	lua_pushnil(L);
	lua_pushfstring(L, LUAMONGO_ERR_CONNECT_FAILED, connectstr, e.what());
	return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int connection_insert(lua_State *L) {
    void *ud = 0;

    ud = luaL_checkudata(L, 1, LUAMONGO_CONNECTION);
    DBClientConnection *connection = *((DBClientConnection **)ud); 

    const char *ns = luaL_checkstring(L, 2);

    try {
	int type = lua_type(L, 3);
	if (type == LUA_TSTRING) {
	    const char *jsonstr = luaL_checkstring(L, 3);
	    connection->insert(ns, fromjson(jsonstr));
	} else if (type == LUA_TTABLE) {
	    BSONObj data;
	    lua_to_bson(L, 3, data);

	    connection->insert(ns, data);
	}
    } catch (std::exception &e) {
	lua_pushnil(L);
	lua_pushfstring(L, LUAMONGO_ERR_INSERT_FAILED, e.what());
	return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int connection_query(lua_State *L) {
    int n = lua_gettop(L);
    void *ud = 0;

    ud = luaL_checkudata(L, 1, LUAMONGO_CONNECTION);
    DBClientConnection *connection = *((DBClientConnection **)ud); 

    const char *ns = luaL_checkstring(L, 2);

    Query query;
    if (n >= 3) {
	try {
	    int type = lua_type(L, 3);
	    if (type == LUA_TSTRING) {
		query = fromjson(luaL_checkstring(L, 3)); 
	    } else if (type == LUA_TTABLE) {
		BSONObj obj;
		lua_to_bson(L, 3, obj);
		query = obj;
	    }
	} catch (std::exception &e) {
	    lua_pushnil(L);
	    lua_pushfstring(L, LUAMONGO_ERR_QUERY_FAILED, e.what());
	    return 2;
	}
    }

    return cursor_create(L, connection, ns, query);
}

static int connection_gc(lua_State *L) {
    void *ud = 0;

    ud = luaL_checkudata(L, 1, LUAMONGO_CONNECTION);
    DBClientConnection *connection = *((DBClientConnection **)ud);

    delete connection;

    return 0;
}

static int connection_tostring(lua_State *L) {
    void *ud = 0;

    ud = luaL_checkudata(L, 1, LUAMONGO_CONNECTION);
    DBClientConnection *connection = *((DBClientConnection **)ud);

    lua_pushfstring(L, "%s: %s", LUAMONGO_CONNECTION,  connection->toString().c_str());

    return 1;
}

int mongo_connection_register(lua_State *L) {
    static const luaL_Reg connection_methods[] = {
	{"connect", connection_connect},
        {"insert", connection_insert},
        {"query", connection_query},
        {NULL, NULL}
    };

    static const luaL_Reg connection_class_methods[] = {
        {"New", connection_new},
        {NULL, NULL}
    };

    luaL_newmetatable(L, LUAMONGO_CONNECTION);
    luaL_register(L, 0, connection_methods);
    lua_pushvalue(L,-1);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, connection_gc);
    lua_setfield(L, -2, "__gc");

    lua_pushcfunction(L, connection_tostring);
    lua_setfield(L, -2, "__tostring");

    luaL_register(L, LUAMONGO_CONNECTION, connection_class_methods);

    return 1;
}
