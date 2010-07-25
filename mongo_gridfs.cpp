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

extern void bson_to_lua(lua_State *L, const BSONObj &obj);
extern int gridfile_create(lua_State *L, GridFile gf);

namespace {
    inline GridFS* userdata_to_gridfs(lua_State* L, int index) {
        void *ud = 0;

        ud = luaL_checkudata(L, index, LUAMONGO_GRIDFS);
        GridFS *gridfs = *((GridFS **)ud);

        return gridfs;
    }
}

/*
 * gridfs, err = mongo.GridFS.New(connection, dbname[, prefix]) 
 */
static int gridfs_new(lua_State *L) {
    int n = lua_gettop(L);
    int resultcount = 1;

    try {
	void *ud = 0;

	ud = luaL_checkudata(L, 1, LUAMONGO_CONNECTION);
	DBClientConnection *connection = *((DBClientConnection **)ud);

	const char *dbname = lua_tostring(L, 2);

        GridFS **gridfs = (GridFS **)lua_newuserdata(L, sizeof(GridFS *));

	if (n >= 3) {
	    const char *prefix = luaL_checkstring(L, 3);

	    *gridfs = new GridFS(*connection, dbname, prefix); 
	} else {
	    *gridfs = new GridFS(*connection, dbname); 
	}

        luaL_getmetatable(L, LUAMONGO_GRIDFS);
        lua_setmetatable(L, -2);
    } catch (std::exception &e) {
        lua_pushnil(L);
        lua_pushfstring(L, LUAMONGO_ERR_GRIDFS_FAILED, e.what());
        resultcount = 2;
    }

    return resultcount;
}

/*
 * gridfile, err = gridfs:find_file(filename)
 */
static int gridfs_find_file(lua_State *L) {
    GridFS *gridfs = userdata_to_gridfs(L, 1);
    int resultcount = 1;

    try {
	GridFile gridfile = gridfs->findFile(luaL_checkstring(L, 2));
	resultcount = gridfile_create(L, gridfile);
    } catch (std::exception &e) {
	lua_pushboolean(L, 0);
	lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_GRIDFS, "find_file", e.what());
        resultcount = 2;
    }

    return resultcount;
}

/*
 * cursor = gridfs:list()
 */
static int gridfs_list(lua_State *L) {
    GridFS *gridfs = userdata_to_gridfs(L, 1);

    DBClientCursor **cursor = (DBClientCursor **)lua_newuserdata(L, sizeof(DBClientCursor *));
    auto_ptr<DBClientCursor> autocursor = gridfs->list();
    *cursor = autocursor.get();
    autocursor.release();

    luaL_getmetatable(L, LUAMONGO_CURSOR);
    lua_setmetatable(L, -2);

    return 1;
}

/*
 * ok, err = gridfs:remove_file(filename)
 */
static int gridfs_remove_file(lua_State *L) {
    int resultcount = 1;

    GridFS *gridfs = userdata_to_gridfs(L, 1);

    const char *filename = luaL_checkstring(L, 2); 

    try {
	gridfs->removeFile(filename);
	lua_pushboolean(L, 1);
    } catch (std::exception &e) {
	lua_pushboolean(L, 0);
	lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_GRIDFS, "remove_file", e.what());
        resultcount = 2;
    }

    return resultcount;
}

/*
 * gridfile, err = gridfs:store_file(filename[, remote_file], content_type]])
 */
static int gridfs_store_file(lua_State *L) {
    int resultcount = 1;

    GridFS *gridfs = userdata_to_gridfs(L, 1);

    const char *filename = luaL_checkstring(L, 2); 
    const char *remote = luaL_optstring(L, 3, ""); 
    const char *content_type = luaL_optstring(L, 4, ""); 

    try {
	BSONObj res = gridfs->storeFile(filename, remote, content_type);
	bson_to_lua(L, res);
    } catch (std::exception &e) {
	lua_pushnil(L);
	lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_GRIDFS, "store_file", e.what());
        resultcount = 2;
    }

    return resultcount;
}


/*
 * __gc
 */
static int gridfs_gc(lua_State *L) {
    GridFS *gridfs = userdata_to_gridfs(L, 1);

    delete gridfs;

    return 0;
}

/*
 * __tostring
 */
static int gridfs_tostring(lua_State *L) {
    GridFS *gridfs = userdata_to_gridfs(L, 1);

    lua_pushfstring(L, "%s: %p", LUAMONGO_GRIDFS, gridfs);

    return 1;
}

int mongo_gridfs_register(lua_State *L) {
    static const luaL_Reg gridfs_methods[] = {
	{"find_file", gridfs_find_file},
	{"list", gridfs_list},
	{"remove_file", gridfs_remove_file},
	{"store_file", gridfs_store_file},
        {NULL, NULL}
    };

    static const luaL_Reg gridfs_class_methods[] = {
	{"New", gridfs_new},
        {NULL, NULL}
    };

    luaL_newmetatable(L, LUAMONGO_GRIDFS);
    luaL_register(L, 0, gridfs_methods);
    lua_pushvalue(L,-1);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, gridfs_gc);
    lua_setfield(L, -2, "__gc");

    lua_pushcfunction(L, gridfs_tostring);
    lua_setfield(L, -2, "__tostring");

    luaL_register(L, LUAMONGO_GRIDFS, gridfs_class_methods);

    return 1;
}
