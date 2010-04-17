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

extern void bson_to_lua(lua_State *L, const BSONObj &obj);

int cursor_create(lua_State *L, DBClientConnection *connection, const char *ns, const Query &query) {
    int resultcount = 1;

    try {
        DBClientCursor **cursor = (DBClientCursor **)lua_newuserdata(L, sizeof(DBClientCursor *));
	auto_ptr<DBClientCursor> autocursor = connection->query(ns, query);
        *cursor = autocursor.get();
	autocursor.release();

        luaL_getmetatable(L, LUAMONGO_CURSOR);
        lua_setmetatable(L, -2);
    } catch (std::exception &e) {
        lua_pushnil(L);
        lua_pushfstring(L, LUAMONGO_ERR_QUERY_FAILED, e.what());
        resultcount = 2;
    }

    return resultcount;
}

static int cursor_next(lua_State *L) {
    void *ud = 0;

    ud = luaL_checkudata(L, 1, LUAMONGO_CURSOR);
    DBClientCursor *cursor = *((DBClientCursor **)ud);

    if (cursor->more()) {
	bson_to_lua(L, cursor->next());	
    } else {
	lua_pushnil(L);
    }

    return 1;
}

static int row_iterator(lua_State *L) {
    void *ud = 0;

    ud = luaL_checkudata(L, lua_upvalueindex(1), LUAMONGO_CURSOR);
    DBClientCursor *cursor = *((DBClientCursor **)ud);

    if (cursor->more()) {
	bson_to_lua(L, cursor->next());	
    } else {
	lua_pushnil(L);
    }

    return 1;
}

static int cursor_rows(lua_State *L) {
    lua_pushcclosure(L, row_iterator, 1);
    return 1;
}

static int cursor_gc(lua_State *L) {
    void *ud = 0;

    ud = luaL_checkudata(L, 1, LUAMONGO_CURSOR);
    DBClientCursor *cursor = *((DBClientCursor **)ud);

    delete cursor;

    return 0;
}

static int cursor_tostring(lua_State *L) {
    return 0;
}

int mongo_cursor_register(lua_State *L) {
    static const luaL_Reg cursor_methods[] = {
	{"next", cursor_next},
	{"rows", cursor_rows},
        {NULL, NULL}
    };

    static const luaL_Reg cursor_class_methods[] = {
        {NULL, NULL}
    };

    luaL_newmetatable(L, LUAMONGO_CURSOR);
    luaL_register(L, 0, cursor_methods);
    lua_pushvalue(L,-1);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, cursor_gc);
    lua_setfield(L, -2, "__gc");

    lua_pushcfunction(L, cursor_tostring);
    lua_setfield(L, -2, "__tostring");

    luaL_register(L, LUAMONGO_CURSOR, cursor_class_methods);

    return 1;
}
