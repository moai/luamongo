#include <client/dbclient.h>

extern "C" {
#include <lua.h>
#include <lauxlib.h>

#if !defined(LUA_VERSION_NUM) || (LUA_VERSION_NUM < 501)
#include <compat-5.1.h>
#endif
};

#include "utils.h"
#include "common.h"

using namespace mongo;

extern int cursor_create(lua_State *L, DBClientBase *connection, const char *ns,
                         const Query &query, int nToReturn, int nToSkip,
                         const BSONObj *fieldsToReturn, int queryOptions, int batchSize);

extern void lua_to_bson(lua_State *L, int stackpos, BSONObj &obj);
extern void bson_to_lua(lua_State *L, const BSONObj &obj);
extern void lua_push_value(lua_State *L, const BSONElement &elem);


DBClientBase* userdata_to_dbclient(lua_State *L, int stackpos)
{
    // adapted from http://www.lua.org/source/5.1/lauxlib.c.html#luaL_checkudata
    void *ud = lua_touserdata(L, stackpos);
    if (ud == NULL)
        luaL_typerror(L, stackpos, "userdata");

    // try Connection
    lua_getfield(L, LUA_REGISTRYINDEX, LUAMONGO_CONNECTION);
    if (lua_getmetatable(L, stackpos))
    {
        if (lua_rawequal(L, -1, -2))
        {
            DBClientConnection *connection = *((DBClientConnection **)ud);
            lua_pop(L, 2);
            return connection;
        }
        lua_pop(L, 2);
    }
    else
        lua_pop(L, 1);

    // try ReplicaSet
    lua_getfield(L, LUA_REGISTRYINDEX, LUAMONGO_REPLICASET);  // get correct metatable
    if (lua_getmetatable(L, stackpos))
    {
        if (lua_rawequal(L, -1, -2))
        {
            DBClientReplicaSet *replicaset = *((DBClientReplicaSet **)ud);
            lua_pop(L, 2); // remove both metatables
            return replicaset;
        }
        lua_pop(L, 2);
    }
    else
        lua_pop(L, 1);

    luaL_typerror(L, stackpos, LUAMONGO_DBCLIENT);
    return NULL; // should never get here
}


/***********************************************************************/
// The following methods are common to all DBClients
// (DBClientConnection and DBClientReplicaSet)
/***********************************************************************/


/*
 * created = db:ensure_index(ns, json_str or lua_table[, unique[, name]])
 */
static int dbclient_ensure_index(lua_State *L) {
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);
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

    bool res = dbclient->ensureIndex(ns, fields, unique, name);

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
static int dbclient_auth(lua_State *L) {
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);

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
    bool success = dbclient->auth(dbname, username, password, errmsg, digestPassword);
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
static int dbclient_is_failed(lua_State *L) {
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);

    bool is_failed = dbclient->isFailed();
    lua_pushboolean(L, is_failed);
    return 1;
}

/*
 * addr = db:get_server_address()
 */
static int dbclient_get_server_address(lua_State *L) {
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);

    std::string address = dbclient->getServerAddress();
    lua_pushstring(L, address.c_str());
    return 1;
}

/*
 * count,err = db:count(ns, lua_table or json_str)
 */
static int dbclient_count(lua_State *L) {
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);
    const char *ns = luaL_checkstring(L, 2);

    int count = 0;
    try {
        BSONObj query;
        int type = lua_type(L, 3);
        if (type == LUA_TSTRING) {
            const char *jsonstr = luaL_checkstring(L, 3);
            query = fromjson(jsonstr);
        } else if (type == LUA_TTABLE) {
            lua_to_bson(L, 3, query);
        }
        count = dbclient->count(ns, query);
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
static int dbclient_insert(lua_State *L) {
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);
    const char *ns = luaL_checkstring(L, 2);

    try {
        int type = lua_type(L, 3);
        if (type == LUA_TSTRING) {
            const char *jsonstr = luaL_checkstring(L, 3);
            dbclient->insert(ns, fromjson(jsonstr));
        } else if (type == LUA_TTABLE) {
            BSONObj data;
            lua_to_bson(L, 3, data);

            dbclient->insert(ns, data);
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
 * ok,err = db:insert_batch(ns, lua_array_of_tables)
 */
static int dbclient_insert_batch(lua_State *L) {
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);
    const char *ns = luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);

    try {
        std::vector<BSONObj> vdata;
        size_t tlen = lua_objlen(L, 3) + 1;
        for (size_t i = 1; i < tlen; ++i) {
            vdata.push_back(BSONObj());
            lua_rawgeti(L, 3, i);
            lua_to_bson(L, 4, vdata.back());
            lua_pop(L, 1);
        }
        dbclient->insert(ns, vdata);
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
static int dbclient_query(lua_State *L) {
    int n = lua_gettop(L);
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);
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

    int res = cursor_create(L, dbclient, ns, query, nToReturn, nToSkip,
                            fieldsToReturn, queryOptions, batchSize);

    if (fieldsToReturn) {
        delete fieldsToReturn;
    }

    return res;
}

/**
 * lua_table,err = db:find_one(ns, lua_table or json_str or query_obj, lua_table or json_str, options)
 */
static int dbclient_find_one(lua_State *L) {
    int n = lua_gettop(L);
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);
    const char *ns = luaL_checkstring(L, 2);

    Query query;
    if (!lua_isnoneornil(L, 3)) {
        try {
            int type = lua_type(L, 3);
            if(type == LUA_TSTRING) {
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
        } catch(std::exception &e) {
            lua_pushnil(L);
            lua_pushfstring(L, LUAMONGO_ERR_FIND_ONE_FAILED, e.what());
            return 2;
        } catch (const char *err) {
            lua_pushnil(L);
            lua_pushstring(L, err);
            return 2;
        }
    }

    const BSONObj *fieldsToReturn = NULL;
    if (!lua_isnoneornil(L, 4)) {
        int type = lua_type(L, 4);

        if (type == LUA_TSTRING) {
            fieldsToReturn = new BSONObj(luaL_checkstring(L, 4));
        } else if (type == LUA_TTABLE) {
            BSONObj obj;
            lua_to_bson(L, 4, obj);
            fieldsToReturn = new BSONObj(obj);
        } else {
            throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
        }
    }

    int queryOptions = luaL_optint(L, 5, 0);

    int retval = 1;
    try {
        BSONObj ret = dbclient->findOne(ns, query, fieldsToReturn, queryOptions);
        bson_to_lua(L, ret);
    } catch (AssertionException &e) {
        lua_pushnil(L);
        lua_pushfstring(L, LUAMONGO_ERR_FIND_ONE_FAILED, e.what());
        retval = 2;
    }

    if (fieldsToReturn) {
        delete fieldsToReturn;
    }
    return retval;
}

/*
 * ok,err = db:remove(ns, lua_table or json_str or query_obj)
 */
static int dbclient_remove(lua_State *L) {
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);
    const char *ns = luaL_checkstring(L, 2);

    try {
        int type = lua_type(L, 3);
        bool justOne = lua_toboolean(L, 4);

        if (type == LUA_TSTRING) {
            const char *jsonstr = luaL_checkstring(L, 3);
            dbclient->remove(ns, fromjson(jsonstr), justOne);
        } else if (type == LUA_TTABLE) {
            BSONObj data;
            lua_to_bson(L, 3, data);

            dbclient->remove(ns, data, justOne);
        } else if (type == LUA_TUSERDATA) {
            Query query;
            void *uq = 0;

            uq = luaL_checkudata(L, 3, LUAMONGO_QUERY);
            query = *(*((Query **)uq));

            dbclient->remove(ns, query, justOne);
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
static int dbclient_update(lua_State *L) {
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);
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

        dbclient->update(ns, query, obj, upsert, multi);
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
 * ok,err = db:drop_collection(ns)
 */
static int dbclient_drop_collection(lua_State *L) {
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);
    const char *ns = luaL_checkstring(L, 2);

    try {
        dbclient->dropCollection(ns);
    } catch (std::exception &e) {
        lua_pushboolean(L, 0);
        lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION, "drop_collection", e.what());
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

/*
 * ok,err = db:drop_index_by_fields(ns, json_str or lua_table)
 */
static int dbclient_drop_index_by_fields(lua_State *L) {
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);
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

        dbclient->dropIndex(ns, keys);
    } catch (std::exception &e) {
        lua_pushboolean(L, 0);
        lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION, "drop_index_by_fields", e.what());
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
 * ok,err = db:drop_index_by_name(ns, index_name)
 */
static int dbclient_drop_index_by_name(lua_State *L) {
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);
    const char *ns = luaL_checkstring(L, 2);

    try {
        dbclient->dropIndex(ns, luaL_checkstring(L, 3));
    } catch (std::exception &e) {
        lua_pushboolean(L, 0);
        lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION, "drop_index_by_name", e.what());
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

/*
 * ok,err = db:drop_indexes(ns)
 */
static int dbclient_drop_indexes(lua_State *L) {
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);
    const char *ns = luaL_checkstring(L, 2);

    try {
        dbclient->dropIndexes(ns);
    } catch (std::exception &e) {
        lua_pushboolean(L, 0);
        lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION, "drop_indexes", e.what());
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

/*
 * res,err = (dbname, jscode[, args_table])
 */
static int dbclient_eval(lua_State *L) {
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);
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

    bool res = dbclient->eval(dbname, jscode, info, retval, args);

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
static int dbclient_exists(lua_State *L) {
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);
    const char *ns = luaL_checkstring(L, 2);

    bool res = dbclient->exists(ns);

    lua_pushboolean(L, res);

    return 1;
}

/*
 * name = db:gen_index_name(json_str or lua_table)
 */
static int dbclient_gen_index_name(lua_State *L) {
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);

    string name = "";

    try {
        int type = lua_type(L, 2);
        if (type == LUA_TSTRING) {
            const char *jsonstr = luaL_checkstring(L, 2);
            name = dbclient->genIndexName(fromjson(jsonstr));
        } else if (type == LUA_TTABLE) {
            BSONObj data;
            lua_to_bson(L, 2, data);

            name = dbclient->genIndexName(data);
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
 * cursor,err = db:get_indexes(ns)
 */
static int dbclient_get_indexes(lua_State *L) {
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);
    const char *ns = luaL_checkstring(L, 2);

    auto_ptr<DBClientCursor> autocursor = dbclient->getIndexes(ns);

    if (!autocursor.get()) {
        lua_pushnil(L);
        lua_pushstring(L, LUAMONGO_ERR_CONNECTION_LOST);
        return 2;
    }

    DBClientCursor **cursor = (DBClientCursor **)lua_newuserdata(L, sizeof(DBClientCursor *));
    *cursor = autocursor.get();
    autocursor.release();

    luaL_getmetatable(L, LUAMONGO_CURSOR);
    lua_setmetatable(L, -2);

    return 1;
}

/*
 * res,err = db:mapreduce(jsmapfunc, jsreducefunc[, query[, output]])
 */
static int dbclient_mapreduce(lua_State *L) {
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);
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

    BSONObj res = dbclient->mapreduce(ns, jsmapfunc, jsreducefunc, query, output);

    bson_to_lua(L, res);

    return 1;
}

/*
 * ok,err = db:reindex(ns);
 */
static int dbclient_reindex(lua_State *L) {
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);
    const char *ns = luaL_checkstring(L, 2);

    try {
        dbclient->reIndex(ns);
    } catch (std::exception &e) {
        lua_pushboolean(L, 0);
        lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION, "reindex", e.what());
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

/*
 * db:reset_index_cache()
 */
static int dbclient_reset_index_cache(lua_State *L) {
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);

    dbclient->resetIndexCache();

    return 0;
}

/*
 * db:get_last_error()
 */
static int dbclient_get_last_error(lua_State *L) {
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);

    string result = dbclient->getLastError();
    lua_pushlstring(L, result.c_str(), result.size());
    return 1;
}

/*
 * db:get_last_error_detailed()
 */
static int dbclient_get_last_error_detailed(lua_State *L) {
    DBClientBase *dbclient = userdata_to_dbclient(L, 1);

    BSONObj res = dbclient->getLastErrorDetailed();
    bson_to_lua(L, res);
    return 1;
}

/*
 * res,err = db:run_command(dbname, lua_table or json_str, options)
 */
static int dbclient_run_command(lua_State *L) {
  DBClientBase *dbclient = userdata_to_dbclient(L, 1);
  const char *ns = luaL_checkstring(L, 2);
  int options = lua_tointeger(L, 4); // if it is invalid it returns 0

  BSONObj command; // arg 3
  try {
    int type = lua_type(L, 3);
    if (type == LUA_TSTRING) {
      const char *jsonstr = luaL_checkstring(L, 3);
      command = fromjson(jsonstr);
    } else if (type == LUA_TTABLE) {
      lua_to_bson(L, 3, command);
    } else {
      throw(LUAMONGO_REQUIRES_JSON_OR_TABLE);
    }

    BSONObj retval;
    bool success = dbclient->runCommand(ns, command, retval, options);
    if ( !success )
      throw "run_command failed";

    bson_to_lua(L, retval );
    return 1;
  } catch (std::exception &e) {
    lua_pushboolean(L, 0);
    lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_CONNECTION,
            "run_command", e.what());
    return 2;
  } catch (const char *err) {
    lua_pushboolean(L, 0);
    lua_pushstring(L, err);
    return 2;
  }
}


// Method registration table for DBClients
extern const luaL_Reg dbclient_methods[] = {
    {"auth", dbclient_auth},
    {"count", dbclient_count},
    {"drop_collection", dbclient_drop_collection},
    {"drop_index_by_fields", dbclient_drop_index_by_fields},
    {"drop_index_by_name", dbclient_drop_index_by_name},
    {"drop_indexes", dbclient_drop_indexes},
    {"ensure_index", dbclient_ensure_index},
    {"eval", dbclient_eval},
    {"exists", dbclient_exists},
    {"find_one", dbclient_find_one},
    {"gen_index_name", dbclient_gen_index_name},
    {"get_indexes", dbclient_get_indexes},
    {"get_last_error", dbclient_get_last_error},
    {"get_last_error_detailed", dbclient_get_last_error_detailed},
    {"get_server_address", dbclient_get_server_address},
    {"insert", dbclient_insert},
    {"insert_batch", dbclient_insert_batch},
    {"is_failed", dbclient_is_failed},
    {"mapreduce", dbclient_mapreduce},
    {"query", dbclient_query},
    {"reindex", dbclient_reindex},
    {"remove", dbclient_remove},
    {"reset_index_cache", dbclient_reset_index_cache},
    {"run_command", dbclient_run_command},
    {"update", dbclient_update},
    {NULL, NULL}
};


