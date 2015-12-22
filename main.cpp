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
#include <client/init.h>
#include <sys/time.h>
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

int mongo_sleep(lua_State *L) {
    double sleeptime = luaL_checknumber(L, 1);
    double seconds   = floor(sleeptime);
    struct timespec req;
    req.tv_sec  = (time_t)seconds;
    req.tv_nsec = (long)((sleeptime-seconds)*1e6);
    nanosleep(&req, 0);
    return 0;
}

int mongo_time(lua_State *L) {
    struct timeval wop;    
    gettimeofday(&wop, 0);
    lua_pushnumber(L, static_cast<double>(wop.tv_sec) +
                   static_cast<double>(wop.tv_usec)*1e-6);
    return 1;
}

struct Initializer {
    mongo::client::GlobalInstance *instance;
    Initializer() : instance(0) { }
    ~Initializer() { delete instance; }
    void init() {
	instance = new mongo::client::GlobalInstance();
	instance->assertInitialized();
    }
};
Initializer initializer;

/*
 *
 * library entry point
 *
 */

extern "C" {

LM_EXPORT int luaopen_mongo(lua_State *L) {
    static const luaL_Reg static_functions[] = {
        {"sleep", mongo_sleep},
        {"time", mongo_time},
        {NULL, NULL}
    };
    
    initializer.init();
    
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
    
    // push the version number and module name
    lua_pushstring(L, LUAMONGO_NAME);
    lua_setfield(L, -2, LUAMONGO_NAME_STRING);
    lua_pushstring(L, LUAMONGO_VERSION);
    lua_setfield(L, -2, LUAMONGO_VERSION_STRING);

    // add static functions
    luaL_setfuncs(L, static_functions, 0);

    return 1;
}

}
