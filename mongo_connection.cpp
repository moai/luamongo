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

extern int cursor_create(lua_State *L, DBClientConnection *connection, const char *ns, const Query &query, int nToReturn, int nToSkip, const BSONObj *fieldsToReturn, int queryOptions, int batchSize);

extern void lua_to_bson(lua_State *L, int stackpos, BSONObj &obj);
extern void bson_to_lua(lua_State *L, const BSONObj &obj);

	
namespace {
inline DBClientConnection* userdata_to_connection(lua_State* L, int index) {
    void *ud = 0;
    ud = luaL_checkudata(L, index, LUAMONGO_CONNECTION);
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
        int rw_timeout;
        if (lua_type(L,1) == LUA_TTABLE) {
            // extract arguments from table
            lua_getfield(L, 1, "auto_reconnect");
            auto_reconnect = lua_toboolean(L, -1);
            lua_getfield(L, 1, "rw_timeout");
            int rw_timeout = lua_tointeger(L, -1);
            lua_pop(L, 2);
        } else {
            auto_reconnect = false;
            rw_timeout = 0;
        }

        DBClientConnection **connection = (DBClientConnection **)lua_newuserdata(L, sizeof(DBClientConnection *));
#if defined(MONGO_PRE_1_5)
        *connection = new DBClientConnection(auto_reconnect, 0);
#else
        *connection = new DBClientConnection(auto_reconnect, 0, rw_timeout);
#endif

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
 * ok,err = db:connect(connection_str)
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
 * ok,err = db:auth({})
 *    accepts a table of parameters:
 *       dbname           database to authenticate (required)
 *       username         username to authenticate against (required)
 *       password         password to authenticate against (required)
 *       digestPassword   set to true if password is pre-digested (default = true)
 *
 */
static int connection_auth(lua_State *L) {
    DBClientConnection *connection = userdata_to_connection(L, 1);

    luaL_checktype(L, 2, LUA_TTABLE);
    lua_getfield(L, 2, "dbname");
    const char *dbname = luaL_checkstring(L, -1);
    lua_getfield(L, 2, "username");
    const char *username = luaL_checkstring(L, -1);
    lua_getfield(L, 2, "password");
    const char *password = luaL_checkstring(L, -1);
    lua_getfield(L, 2, "digestPassword");
    bool digestPassword = lua_isnil(L, -1) ? true : lua_toboolean(L, -1);
    lua_pop(L, 4);

    std::string errmsg;
    bool success = connection->auth(dbname, username, password, errmsg, digestPassword);
    if (!success) {
        lua_pushnil(L);
        lua_pushfstring(L, LUAMONGO_ERR_CONNECTION_FAILED, errmsg.c_str());
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}
        
/*
 * is_failed = db:is_failed()
 */
static int connection_is_failed(lua_State *L) {
    DBClientConnection *connection = userdata_to_connection(L, 1);

    bool is_failed = connection->isFailed();
    lua_pushboolean(L, is_failed);
    return 1;
}

/*
 * addr = db:get_server_address()
 */
static int connection_get_server_address(lua_State *L) {
    DBClientConnection *connection = userdata_to_connection(L, 1);

    std::string address = connection->getServerAddress();
    lua_pushstring(L, address.c_str());
    return 1;
}

/*
 * count,err = db:count(ns)
 */
static int connection_count(lua_State *L) {
    DBClientConnection *connection = userdata_to_connection(L, 1);
    const char *ns = luaL_checkstring(L, 2);

    int count = 0;
    try {
        count = connection->count(ns);
    } catch (std::exception &e) {
        lua_pushnil(L);
        lua_pushfstring(L, LUAMONGO_ERR_COUNT_FAILED, e.what());
        return 2;
    }

    lua_pushinteger(L, count);
    return 1;
}

/*
 * ok,err = db:insert(ns, lua_table or json_str)
 */
static int connection_insert(lua_State *L) {
    DBClientConnection *connection = userdata_to_connection(L, 1);
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
        } else {
            throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
        }
    } catch (std::exception &e) {
        lua_pushnil(L);
        lua_pushfstring(L, LUAMONGO_ERR_INSERT_FAILED, e.what());
        return 2;
    } catch (const char *err) {
        lua_pushnil(L);
        lua_pushstring(L, err);
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

/*
 * cursor,err = db:query(ns, lua_table or json_str or query_obj, limit, skip, lua_table or json_str, options, batchsize)
 */
static int connection_query(lua_State *L) {
    int n = lua_gettop(L);
    DBClientConnection *connection = userdata_to_connection(L, 1);
    const char *ns = luaL_checkstring(L, 2);

    Query query;
    if (!lua_isnoneornil(L, 3)) {
        try {
            int type = lua_type(L, 3);
            if (type == LUA_TSTRING) {
                query = fromjson(luaL_checkstring(L, 3)); 
            } else if (type == LUA_TTABLE) {
                BSONObj obj;
                lua_to_bson(L, 3, obj);
                query = obj;
            } else if (type == LUA_TUSERDATA) {
                void *uq = 0;

                uq = luaL_checkudata(L, 3, LUAMONGO_QUERY);
                query = *(*((Query **)uq)); 
            } else {
                throw(LUAMONGO_REQUIRES_QUERY);
            }
        } catch (std::exception &e) {
            lua_pushnil(L);
            lua_pushfstring(L, LUAMONGO_ERR_QUERY_FAILED, e.what());
            return 2;
        } catch (const char *err) {
            lua_pushnil(L);
            lua_pushstring(L, err);
            return 2;
        }
    }

    int nToReturn = luaL_optint(L, 4, 0);
    int nToSkip = luaL_optint(L, 5, 0);

    const BSONObj *fieldsToReturn = NULL;
    if (!lua_isnoneornil(L, 6)) {
	fieldsToReturn = new BSONObj();

	int type = lua_type(L, 6);

	if (type == LUA_TSTRING) {
	    fieldsToReturn = new BSONObj(luaL_checkstring(L, 6));
	} else if (type == LUA_TTABLE) {
	    BSONObj obj;
	    lua_to_bson(L, 6, obj);
	    fieldsToReturn = new BSONObj(obj);
	} else {
	    throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
	}
    }

    int queryOptions = luaL_optint(L, 7, 0);
    int batchSize = luaL_optint(L, 8, 0);

    int res = cursor_create(L, connection, ns, query, nToReturn, nToSkip, fieldsToReturn, queryOptions, batchSize);

    if (fieldsToReturn) {
	delete fieldsToReturn;
    }

    return res;
}

/*
 * ok,err = db:remove(ns, lua_table or json_str or query_obj)
 */
static int connection_remove(lua_State *L) {
    DBClientConnection *connection = userdata_to_connection(L, 1);
    const char *ns = luaL_checkstring(L, 2);

    try {
        int type = lua_type(L, 3);
        bool justOne = lua_toboolean(L, 4);

        if (type == LUA_TSTRING) {
            const char *jsonstr = luaL_checkstring(L, 3);
            connection->remove(ns, fromjson(jsonstr), justOne);
        } else if (type == LUA_TTABLE) {
            BSONObj data;
            lua_to_bson(L, 3, data);

            connection->remove(ns, data, justOne);
        } else if (type == LUA_TUSERDATA) {
	    Query query;
            void *uq = 0;

            uq = luaL_checkudata(L, 3, LUAMONGO_QUERY);
            query = *(*((Query **)uq)); 

	    connection->remove(ns, query, justOne);
        } else {
            throw(LUAMONGO_REQUIRES_QUERY);
        }
    } catch (std::exception &e) {
        lua_pushnil(L);
        lua_pushfstring(L, LUAMONGO_ERR_REMOVE_FAILED, e.what());
        return 2;
    } catch (const char *err) {
        lua_pushnil(L);
        lua_pushstring(L, err);
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

/*
 * ok,err = db:update(ns, lua_table or json_str or query_obj, lua_table or json_str, upsert, multi)
 */
static int connection_update(lua_State *L) {
    DBClientConnection *connection = userdata_to_connection(L, 1);
    const char *ns = luaL_checkstring(L, 2);

    try {
        int type_query = lua_type(L, 3);
        int type_obj = lua_type(L, 4);

        bool upsert = lua_toboolean(L, 5);
        bool multi = lua_toboolean(L, 6);

        Query query;
        BSONObj obj;

        if (type_query == LUA_TSTRING) {
            const char *jsonstr = luaL_checkstring(L, 3);
            query = fromjson(jsonstr);
        } else if (type_query == LUA_TTABLE) {
	    BSONObj q;

            lua_to_bson(L, 3, q);
	    query = q;
        } else if (type_query == LUA_TUSERDATA) {
            void *uq = 0;

            uq = luaL_checkudata(L, 3, LUAMONGO_QUERY);
            query = *(*((Query **)uq)); 
        } else {
            throw(LUAMONGO_REQUIRES_QUERY);
        }
    
        if (type_obj == LUA_TSTRING) {
            const char *jsonstr = luaL_checkstring(L, 4);
            obj = fromjson(jsonstr);
        } else if (type_obj == LUA_TTABLE) {
            lua_to_bson(L, 4, obj);
        } else {
            throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
        }

        connection->update(ns, query, obj, upsert, multi);
    } catch (std::exception &e) {
        lua_pushnil(L);
        lua_pushfstring(L, LUAMONGO_ERR_UPDATE_FAILED, e.what());
        return 2;
    } catch (const char *err) {
        lua_pushnil(L);
        lua_pushstring(L, err);
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
        {"auth", connection_auth},
        {"is_failed", connection_is_failed},
        {"get_server_address", connection_get_server_address},
        {"count", connection_count},
        {"insert", connection_insert},
        {"query", connection_query},
        {"remove", connection_remove},
        {"update", connection_update},
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
