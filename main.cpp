/*
 * Copyright (c) 2009 Neil Richardson (nrich@iinet.net.au)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#if !defined(LUA_VERSION_NUM) || (LUA_VERSION_NUM < 501)
#include <compat-5.1.h>
#endif
};

#include <iostream>
#include <client/dbclient.h>

#include "utils.h"
#include "common.h"

extern int mongo_bsontypes_register(lua_State *L);
extern int mongo_connection_register(lua_State *L);
extern int mongo_replicaset_register(lua_State *L);
extern int mongo_cursor_register(lua_State *L);
extern int mongo_query_register(lua_State *L);
extern int mongo_gridfs_register(lua_State *L);
extern int mongo_gridfile_register(lua_State *L);
extern int mongo_gridfschunk_register(lua_State *L);

/*
 *
 * library entry point
 *
 */

extern "C" {

LM_EXPORT int luaopen_mongo(lua_State *L) {
    mongo_bsontypes_register(L);
    mongo_connection_register(L);
    mongo_replicaset_register(L);
    mongo_cursor_register(L);
    mongo_query_register(L);

    mongo_gridfs_register(L);
    mongo_gridfile_register(L);
    mongo_gridfschunk_register(L);

    /*
     * push the created table to the top of the stack
     * so "mongo = require('mongo')" works
     */
    lua_getglobal(L, LUAMONGO_ROOT);

    return 1;
}

}
