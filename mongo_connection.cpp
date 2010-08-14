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
extern void lua_push_value(lua_State *L, const BSONElement &elem);
	
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
 * created = db:ensure_index(ns, json_str or lua_table[, unique[, name]])
 */
static int connection_ensure_index(lua_State *L) {
    DBClientConnection *connection = userdata_to_connection(L, 1);
    const char *ns = luaL_checkstring(L, 2);
    BSONObj fields;
 
    try {
        int type = lua_type(L, 3);
        if (type == LUA_TSTRING) {
            const char *jsonstr = luaL_checkstring(L, 3);
            fields = fromjson(jsonstr);
        } else if (type == LUA_TTABLE) {
            lua_to_bson(L, 3, fields);
        } else {
            throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
        }
    } catch (std::exception &e) {
	lua_pushboolean(L, 0);
        lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION, "ensure_index", e.what());
        return 2;
    } catch (const char *err) {
	lua_pushboolean(L, 0);
        lua_pushstring(L, err);
        return 2;
    }

    bool unique = lua_toboolean(L, 4);
    const char *name = luaL_optstring(L, 5, ""); 

    bool res = connection->ensureIndex(ns, fields, unique, name);

    lua_pushboolean(L, res);
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
        lua_pushboolean(L, 0);
        lua_pushfstring(L, LUAMONGO_ERR_INSERT_FAILED, e.what());
        return 2;
    } catch (const char *err) {
        lua_pushboolean(L, 0);
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
        lua_pushboolean(L, 0);
        lua_pushfstring(L, LUAMONGO_ERR_REMOVE_FAILED, e.what());
        return 2;
    } catch (const char *err) {
        lua_pushboolean(L, 0);
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
        lua_pushboolean(L, 0);
        lua_pushfstring(L, LUAMONGO_ERR_UPDATE_FAILED, e.what());
        return 2;
    } catch (const char *err) {
        lua_pushboolean(L, 0);
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

/*
 * ok, err = db:drop_collection(ns)
 */

static int connection_drop_collection(lua_State *L) {
    DBClientConnection *connection = userdata_to_connection(L, 1);
    const char *ns = luaL_checkstring(L, 2);
    
    bool res = connection->dropCollection(ns);

    if (!res) {
	string lasterr = connection->getLastError();

        lua_pushboolean(L, 0);
        lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION, "drop_collection", lasterr.c_str());

	return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

/*
 * ok, err = db:drop_index_by_fields(ns, json_str or lua_table)
 */
static int connection_drop_index_by_fields(lua_State *L) {
    DBClientConnection *connection = userdata_to_connection(L, 1);
    const char *ns = luaL_checkstring(L, 2);

    BSONObj keys;

    try {
        int type = lua_type(L, 3);
        if (type == LUA_TSTRING) {
            const char *jsonstr = luaL_checkstring(L, 3);
            keys = fromjson(jsonstr);
        } else if (type == LUA_TTABLE) {
            lua_to_bson(L, 3, keys);
        } else {
            throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
        }
    } catch (std::exception &e) {
	lua_pushboolean(L, 0);
        lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION, "drop_index_by_fields", e.what());
        return 2;
    } catch (const char *err) {
	lua_pushboolean(L, 0);
        lua_pushstring(L, err);
        return 2;
    }

    connection->dropIndex(ns, keys);

    lua_pushboolean(L, 1);
    return 1;
}

/*
 * db:drop_index_by_name(ns, index_name)
 */
static int connection_drop_index_by_name(lua_State *L) {
    DBClientConnection *connection = userdata_to_connection(L, 1);
    const char *ns = luaL_checkstring(L, 2);

    connection->dropIndex(ns, luaL_checkstring(L, 3));

    return 0;
}

/*
 * db:drop_indexes(ns)
 */
static int connection_drop_indexes(lua_State *L) {
    DBClientConnection *connection = userdata_to_connection(L, 1);
    const char *ns = luaL_checkstring(L, 2);

    connection->dropIndexes(ns);

    return 0;
}

/*
 * res, err = (dbname, jscode[, args_table]) 
 */
static int connection_eval(lua_State *L) {
    DBClientConnection *connection = userdata_to_connection(L, 1);
    const char *dbname = luaL_checkstring(L, 2);
    const char *jscode = luaL_checkstring(L, 3);

    BSONObj info; 
    BSONElement retval;
    BSONObj *args = NULL; 
    if (!lua_isnoneornil(L, 4)) {
	try {
	    int type = lua_type(L, 4);

	    if (type == LUA_TSTRING) {
		args = new BSONObj(luaL_checkstring(L, 4));
	    } else if (type == LUA_TTABLE) {
		BSONObj obj;
		lua_to_bson(L, 4, obj);

		args = new BSONObj(obj);
	    } else {
		throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
	    }
	} catch (std::exception &e) {
	    lua_pushnil(L);
	    lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION, "eval", e.what());
	    return 2;
	} catch (const char *err) {
	    lua_pushnil(L);
	    lua_pushstring(L, err);
	    return 2;
	}
    }

    bool res = connection->eval(dbname, jscode, info, retval, args);

    if (!res) {
	lua_pushboolean(L, 0);
	lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION, "eval", info["errmsg"].str().c_str());

	return 2;
    }

    if (args) {
	delete args;
    }

    lua_push_value(L, retval);
    return 1;
}

/*
 * bool = db:exists(ns)
 */
static int connection_exists(lua_State *L) {
    DBClientConnection *connection = userdata_to_connection(L, 1);
    const char *ns = luaL_checkstring(L, 2);

    bool res = connection->exists(ns);

    lua_pushboolean(L, res);

    return 1;
}

/*
 * name = db:gen_index_name(json_str or lua_table)
 */
static int connection_gen_index_name(lua_State *L) {
    DBClientConnection *connection = userdata_to_connection(L, 1);

    string name = ""; 

    try {
        int type = lua_type(L, 2);
        if (type == LUA_TSTRING) {
            const char *jsonstr = luaL_checkstring(L, 2);
            name = connection->genIndexName(fromjson(jsonstr));
        } else if (type == LUA_TTABLE) {
            BSONObj data;
            lua_to_bson(L, 2, data);

            name = connection->genIndexName(data);
        } else {
            throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
        }
    } catch (std::exception &e) {
        lua_pushnil(L);
        lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION, "gen_index_name", e.what());
        return 2;
    } catch (const char *err) {
        lua_pushnil(L);
        lua_pushstring(L, err);
        return 2;
    }

    lua_pushstring(L, name.c_str());
    return 1;
}

/*
 * cursor = db:get_indexes(ns)
 */
static int connection_get_indexes(lua_State *L) {
    DBClientConnection *connection = userdata_to_connection(L, 1);
    const char *ns = luaL_checkstring(L, 2);

    auto_ptr<DBClientCursor> autocursor = connection->getIndexes(ns);

    DBClientCursor **cursor = (DBClientCursor **)lua_newuserdata(L, sizeof(DBClientCursor *));
    *cursor = autocursor.get();
    autocursor.release();

    luaL_getmetatable(L, LUAMONGO_CURSOR);
    lua_setmetatable(L, -2);

    return 1;
}

/*
 * res, err = db:mapreduce(jsmapfunc, jsreducefunc[, query[, output]]) 
 */
static int connection_mapreduce(lua_State *L) {
    DBClientConnection *connection = userdata_to_connection(L, 1);
    const char *ns = luaL_checkstring(L, 2);
    const char *jsmapfunc = luaL_checkstring(L, 3);
    const char *jsreducefunc = luaL_checkstring(L, 4);

    BSONObj query;
    if (!lua_isnoneornil(L, 5)) {
	try {
	    int type = lua_type(L, 5);
	    if (type == LUA_TSTRING) {
		const char *jsonstr = luaL_checkstring(L, 5);
		query = fromjson(jsonstr);
	    } else if (type == LUA_TTABLE) {
		lua_to_bson(L, 5, query);
	    } else {
		throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
	    }
	} catch (std::exception &e) {
	    lua_pushnil(L);
	    lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION, "mapreduce", e.what());
	    return 2;
	} catch (const char *err) {
	    lua_pushnil(L);
	    lua_pushstring(L, err);
	    return 2;
	}
    }

    const char *output = luaL_optstring(L, 6, ""); 

    BSONObj res = connection->mapreduce(ns, jsmapfunc, jsreducefunc, query, output);

    bson_to_lua(L, res);

    return 1;
}

/*
 * db:reindex(ns);
 */
static int connection_reindex(lua_State *L) {
    DBClientConnection *connection = userdata_to_connection(L, 1);
    const char *ns = luaL_checkstring(L, 2);

    connection->reIndex(ns);

    return 0;
}

/*
 * db:reset_index_cache()
 */
static int connection_reset_index_cache(lua_State *L) {
    DBClientConnection *connection = userdata_to_connection(L, 1);

    connection->resetIndexCache();

    return 0;
}

int mongo_connection_register(lua_State *L) {
    static const luaL_Reg connection_methods[] = {
        {"auth", connection_auth},
        {"connect", connection_connect},
        {"count", connection_count},
	{"drop_collection", connection_drop_collection},
	{"drop_index_by_fields", connection_drop_index_by_fields},
	{"drop_index_by_name", connection_drop_index_by_name},
	{"drop_indexes", connection_drop_indexes},
        {"ensure_index", connection_ensure_index},
        {"eval", connection_eval},
        {"exists", connection_exists},
	{"gen_index_name", connection_gen_index_name},
	{"get_indexes", connection_get_indexes},
        {"get_server_address", connection_get_server_address},
        {"insert", connection_insert},
        {"is_failed", connection_is_failed},
        {"mapreduce", connection_mapreduce},
        {"query", connection_query},
        {"reindex", connection_reindex},
        {"remove", connection_remove},
        {"reset_index_cache", connection_reset_index_cache},
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
