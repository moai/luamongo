/*
 * Copyright (c) 2014 Francisco Zamora-Martinez (pakozm@gmail.com)
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
extern int mongo_gridfilebuilder_register(lua_State *L);

/*
 *
 * library entry point
 *
 */

extern "C" {

LM_EXPORT int luaopen_mongo(lua_State *L) {
    // bsontypes is the root table
    mongo_bsontypes_register(L);
    
    // LUAMONGO_CONNECTION
    mongo_connection_register(L);
    lua_setfield(L, -2, LUAMONGO_CONNECTION);
    
    // LUAMONGO_REPLICASET
    mongo_replicaset_register(L);
    lua_setfield(L, -2, LUAMONGO_REPLICASET);
    
    // LUAMONGO_CURSOR
    mongo_cursor_register(L);
    lua_setfield(L, -2, LUAMONGO_CURSOR);
    
    // LUAMONGO_QUERY
    mongo_query_register(L);
    lua_setfield(L, -2, LUAMONGO_QUERY);
    
    // LUAMONGO_GRIDFS
    mongo_gridfs_register(L);
    lua_setfield(L, -2, LUAMONGO_GRIDFS);
    
    // LUAMONGO_GRIDFILE
    mongo_gridfile_register(L);
    lua_setfield(L, -2, LUAMONGO_GRIDFILE);
    
    // LUAMONGO_GRIDFSCHUNK
    mongo_gridfschunk_register(L);
    lua_setfield(L, -2, LUAMONGO_GRIDFSCHUNK);

    // LUAMONGO_GRIDFILEBUILDER
    mongo_gridfilebuilder_register(L);
    lua_setfield(L, -2, LUAMONGO_GRIDFILEBUILDER);

    /*
     * push the created table to the top of the stack
     * so "mongo = require('mongo')" works
     */
    // lua_getglobal(L, LUAMONGO_ROOT);

    return 1;
}

}
