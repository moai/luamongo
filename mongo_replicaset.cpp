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

extern const luaL_Reg dbclient_methods[];

namespace {
inline DBClientReplicaSet* userdata_to_replicaset(lua_State* L, int index) {
    void *ud = luaL_checkudata(L, index, LUAMONGO_REPLICASET);
    DBClientReplicaSet *replicaset = *((DBClientReplicaSet **)ud);
    return replicaset;
}

} // anonymous namespace


/*
 * db,err = mongo.ReplicaSet.New(name, {hostAndPort1, ...)
 */
static int replicaset_new(lua_State *L) {
    luaL_checktype(L, 1, LUA_TSTRING);
    luaL_checktype(L, 2, LUA_TTABLE);

    int resultcount = 1;
    try {
        const char *rs_name = luaL_checkstring(L, 1);

        std::vector<mongo::HostAndPort> rs_servers;
        int i = 1;
        while (1) {
            lua_pushinteger(L, i++);
            lua_gettable(L, 2);
            const char* hostAndPort = lua_tostring(L, -1);
            lua_pop(L, 1);
            if (!hostAndPort)
                break;
            HostAndPort hp(hostAndPort);
            rs_servers.push_back(hp);
        }

        DBClientReplicaSet **replicaset = (DBClientReplicaSet **)lua_newuserdata(L, sizeof(DBClientReplicaSet *));
        *replicaset = new DBClientReplicaSet(rs_name, rs_servers);

        luaL_getmetatable(L, LUAMONGO_REPLICASET);
        lua_setmetatable(L, -2);
    } catch (std::exception &e) {
        lua_pushnil(L);
        lua_pushfstring(L, LUAMONGO_ERR_REPLICASET_FAILED, e.what());
        resultcount = 2;
    }

    return resultcount;
}

/*
 * ok,err = replicaset:connect()
 */
static int replicaset_connect(lua_State *L) {
    DBClientReplicaSet *replicaset = userdata_to_replicaset(L, 1);

    try {
        replicaset->connect();
    } catch (std::exception &e) {
        lua_pushnil(L);
        lua_pushfstring(L, LUAMONGO_ERR_CONNECT_FAILED, LUAMONGO_REPLICASET, e.what());
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}


/*
 * __gc
 */
static int replicaset_gc(lua_State *L) {
    DBClientReplicaSet *replicaset = userdata_to_replicaset(L, 1);
    delete replicaset;
    return 0;
}

/*
 * __tostring
 */
static int replicaset_tostring(lua_State *L) {
    DBClientReplicaSet *replicaset = userdata_to_replicaset(L, 1);
    lua_pushfstring(L, "%s: %s", LUAMONGO_REPLICASET, replicaset->toString().c_str());
    return 1;
}


int mongo_replicaset_register(lua_State *L) {
    static const luaL_Reg replicaset_methods[] = {
        {"connect", replicaset_connect},
        {NULL, NULL}
    };

    static const luaL_Reg replicaset_class_methods[] = {
        {"New", replicaset_new},
        {NULL, NULL}
    };

    luaL_newmetatable(L, LUAMONGO_REPLICASET);
    luaL_register(L, NULL, dbclient_methods);
    luaL_register(L, NULL, replicaset_methods);
    lua_pushvalue(L,-1);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, replicaset_gc);
    lua_setfield(L, -2, "__gc");

    lua_pushcfunction(L, replicaset_tostring);
    lua_setfield(L, -2, "__tostring");

    luaL_register(L, LUAMONGO_REPLICASET, replicaset_class_methods);

    return 1;
}

