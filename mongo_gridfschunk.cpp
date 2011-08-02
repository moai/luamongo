#include <iostream>
#include <client/dbclient.h>
#include <client/gridfs.h>

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

namespace {
    inline GridFSChunk* userdata_to_gridfschunk(lua_State* L, int index) {
        void *ud = 0;

        ud = luaL_checkudata(L, index, LUAMONGO_GRIDFSCHUNK);
        GridFSChunk *chunk = *((GridFSChunk **)ud);

        return chunk;
    }
}

/*
 * str = chunk:data()
 */
static int gridfschunk_data(lua_State *L) {
    GridFSChunk *chunk = userdata_to_gridfschunk(L, 1);
    int len;

    const char *data = chunk->data(len);

    lua_pushlstring(L, data, len);

    return 1;
}

/*
 * length = chunk:len()
 * __len
 */
static int gridfschunk_len(lua_State *L) {
    GridFSChunk *chunk = userdata_to_gridfschunk(L, 1);

    int len = chunk->len();

    lua_pushinteger(L, len);

    return 1;
}


/*
 * __gc
 */
static int gridfschunk_gc(lua_State *L) {
    GridFSChunk *chunk = userdata_to_gridfschunk(L, 1);

    delete chunk;

    return 0;
}

/*
 * __tostring
 */
static int gridfschunk_tostring(lua_State *L) {
    GridFSChunk *chunk = userdata_to_gridfschunk(L, 1);

    lua_pushfstring(L, "%s: %p", LUAMONGO_GRIDFSCHUNK, chunk);

    return 1;
}

int mongo_gridfschunk_register(lua_State *L) {
    static const luaL_Reg gridfschunk_methods[] = {
        {"data", gridfschunk_data},
        {"len", gridfschunk_len},
        {NULL, NULL}
    };

    luaL_newmetatable(L, LUAMONGO_GRIDFSCHUNK);
    luaL_register(L, 0, gridfschunk_methods);
    lua_pushvalue(L,-1);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, gridfschunk_gc);
    lua_setfield(L, -2, "__gc");

    lua_pushcfunction(L, gridfschunk_tostring);
    lua_setfield(L, -2, "__tostring");

    lua_pushcfunction(L, gridfschunk_len);
    lua_setfield(L, -2, "__len");

    //luaL_register(L, LUAMONGO_GRIDFSCHUNK, gridfschunk_class_methods);

    return 1;
}
