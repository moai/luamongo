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
inline DBClientConnection* userdata_to_connection(lua_State* L, int index) {
    void *ud = luaL_checkudata(L, index, LUAMONGO_CONNECTION);
    DBClientConnection *connection = *((DBClientConnection **)ud);
    return connection;
}

} // anonymous namespace

/*
 * db,err = mongo.Connection.New({})
 *    accepts an optional table of features:
 *       auto_reconnect   (default = false)
 *       rw_timeout       (default = 0) (mongo >= v1.5)
 */
static int connection_new(lua_State *L) {
    int resultcount = 1;

    try {
        bool auto_reconnect;
        double rw_timeout=0;
        if (lua_type(L,1) == LUA_TTABLE) {
            // extract arguments from table
            lua_getfield(L, 1, "auto_reconnect");
            auto_reconnect = lua_toboolean(L, -1);
            lua_getfield(L, 1, "rw_timeout");
            rw_timeout = lua_tonumber(L, -1);
            lua_pop(L, 2);
        } else {
            auto_reconnect = false;
            rw_timeout = 0;
        }

        DBClientConnection **connection = (DBClientConnection **)lua_newuserdata(L, sizeof(DBClientConnection *));
        *connection = new DBClientConnection(auto_reconnect, 0, rw_timeout);

        luaL_getmetatable(L, LUAMONGO_CONNECTION);
        lua_setmetatable(L, -2);
    } catch (std::exception &e) {
        lua_pushnil(L);
        lua_pushfstring(L, LUAMONGO_ERR_CONNECTION_FAILED, e.what());
        resultcount = 2;
    }

    return resultcount;
}


/*
 * ok,err = connection:connect(connection_str)
 */
static int connection_connect(lua_State *L) {
    DBClientConnection *connection = userdata_to_connection(L, 1);
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


/*
 * __gc
 */
static int connection_gc(lua_State *L) {
    DBClientConnection *connection = userdata_to_connection(L, 1);
    delete connection;
    return 0;
}

/*
 * __tostring
 */
static int connection_tostring(lua_State *L) {
    DBClientConnection *connection = userdata_to_connection(L, 1);
    lua_pushfstring(L, "%s: %s", LUAMONGO_CONNECTION,  connection->toString().c_str());
    return 1;
}


int mongo_connection_register(lua_State *L) {
    static const luaL_Reg connection_methods[] = {
        {"connect", connection_connect},
        {NULL, NULL}
    };

    static const luaL_Reg connection_class_methods[] = {
        {"New", connection_new},
        {NULL, NULL}
    };

    luaL_newmetatable(L, LUAMONGO_CONNECTION);
    luaL_register(L, NULL, dbclient_methods);
    luaL_register(L, NULL, connection_methods);
    lua_pushvalue(L,-1);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, connection_gc);
    lua_setfield(L, -2, "__gc");

    lua_pushcfunction(L, connection_tostring);
    lua_setfield(L, -2, "__tostring");

    luaL_register(L, LUAMONGO_CONNECTION, connection_class_methods);

    return 1;
}

