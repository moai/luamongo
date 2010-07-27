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

#include "common.h"

using namespace mongo;

void push_bsontype_table(lua_State* L, mongo::BSONType bsontype);
extern const char *bson_name(int type);

static int bson_type_Date(lua_State *L) {
    push_bsontype_table(L, mongo::Date);
    lua_pushvalue(L, 1);
    lua_rawseti(L, -2, 1); // t[1] = function arg #1
    return 1;
}

static int bson_type_Timestamp(lua_State*L) {
    push_bsontype_table(L, mongo::Timestamp);
    // no arg
    return 1;
}

static int bson_type_RegEx(lua_State *L) {
    push_bsontype_table(L, mongo::RegEx);
    lua_pushvalue(L, 1);
    lua_rawseti(L, -2, 1); // t[1] = function arg #1
    lua_pushvalue(L, 2);
    lua_rawseti(L, -2, 2); // set t[2] = function arg #2
    return 1;
}

static int bson_type_NumberInt(lua_State *L) {
    push_bsontype_table(L, mongo::NumberInt);
    lua_pushvalue(L, 1);
    lua_rawseti(L, -2, 1); // t[1] = function arg #1
    return 1;
}

static int bson_type_Symbol(lua_State *L) {
    push_bsontype_table(L, mongo::Symbol);
    lua_pushvalue(L, 1);
    lua_rawseti(L, -2, 1); // t[1] = function arg #1
    return 1;
}

static int integer_value(lua_State *L) {
    int n = lua_gettop(L);
    int returncount = 1;

    lua_rawgeti(L, 1, 1);

    if (n > 1) { 
	lua_pushinteger(L, luaL_checkint(L, 2));
	lua_rawseti(L, 1, 1);
	returncount = 0;
    } else {
	lua_pushinteger(L, luaL_checkint(L, -1));
    }

    return returncount;
}

static int number_value(lua_State *L) {
    int n = lua_gettop(L);
    int returncount = 1;

    lua_rawgeti(L, 1, 1);

    if (n > 1) { 
	lua_pushnumber(L, luaL_checknumber(L, 2));
	lua_rawseti(L, 1, 1);
	returncount = 0;
    } else {
	lua_pushnumber(L, luaL_checknumber(L, -1));
    }

    return returncount;
}

static int string_value(lua_State *L) {
    int n = lua_gettop(L);
    int returncount = 1;

    lua_rawgeti(L, 1, 1);

    if (n > 1) { 
	lua_pushstring(L, luaL_checkstring(L, 2));
	lua_rawseti(L, 1, 1);
	returncount = 0;
    } else {
	lua_pushstring(L, luaL_checkstring(L, -1));
    }

    return returncount;
}

static int stringpair_value(lua_State *L) {
    int n = lua_gettop(L);
    int returncount = 2;

    lua_rawgeti(L, 1, 1);
    lua_rawgeti(L, 1, 2);

    if (n > 1) { 
	lua_pushstring(L, luaL_checkstring(L, 2));
	lua_rawseti(L, 1, 1);
	lua_pushstring(L, luaL_checkstring(L, 3));
	lua_rawseti(L, 1, 2);
	returncount = 0;
    } else {
	lua_pushstring(L, luaL_checkstring(L, -2));
	lua_pushstring(L, luaL_checkstring(L, -2));
    }

    return returncount;
}

static int generic_tostring(lua_State *L) {
    lua_rawgeti(L, 1, 1);

    //lua_pushstring(L, lua_tostring(L, -1));
    lua_pushstring(L, luaL_optstring(L, -1, "nil"));

    return 1;
}

static int date_tostring(lua_State *L) {
    char datestr[64];

    lua_rawgeti(L, 1, 1);

    time_t t = (time_t)(lua_tonumber(L, -1)/1000);

#if defined(_WIN32)
    ctime_s(datestr, 64, &t);
#else
    ctime_r(&t,datestr);
#endif

    datestr[24] = 0; // don't want the \n

    lua_pushstring(L, datestr);

    return 1;
}

static int regex_tostring(lua_State *L) {
    lua_rawgeti(L, 1, 1);
    lua_rawgeti(L, 1, 2);

    lua_pushfstring(L, "/%s/%s", lua_tostring(L, -2),  lua_tostring(L, -1));

    return 1;
}


// TODO:
//    all of this should be in Lua so it can get JIT wins
//    bind the bson typeids
//    each type could have its cached metatable (from registry)

// all these types are represented as tables
// the metatable entry __bsontype dictates the type
// the t[1] represents the object itself, with some types using other fields

void push_bsontype_table(lua_State* L, mongo::BSONType bsontype) {
    lua_newtable(L);
    lua_newtable(L);

    lua_pushstring(L, "__bsontype");
    lua_pushinteger(L, bsontype);
    lua_settable(L, -3);

    lua_pushstring(L, "__call");
    switch(bsontype) {
	case mongo::NumberInt:
	    lua_pushcfunction(L, integer_value);
	    break;
	case mongo::Date:
	case mongo::Timestamp:
	    lua_pushcfunction(L, number_value);
	    break;
	case mongo::Symbol:
	    lua_pushcfunction(L, string_value);
	    break;
	case mongo::RegEx:
	    lua_pushcfunction(L, stringpair_value);
	    break;
    }
    lua_settable(L, -3);

    lua_pushstring(L, "__tostring");
    switch(bsontype) {
	case mongo::NumberInt:
	case mongo::Timestamp:
	case mongo::Symbol:
	    lua_pushcfunction(L, generic_tostring);
	    break;
	case mongo::Date:
	    lua_pushcfunction(L, date_tostring);
	    break;
	case mongo::RegEx:
	    lua_pushcfunction(L, regex_tostring);
	    break;
    }
    lua_settable(L, -3);

    lua_setmetatable(L, -2);
}

/*
 * typename = mongo.type(obj)
 */
static int bson_type_name(lua_State *L) {
    if (lua_istable(L, 1)) {
	int bsontype_found = luaL_getmetafield(L, 1, "__bsontype");

	if (bsontype_found) {
            int bson_type = lua_tointeger(L, -1);
            lua_pop(L, 1);

	    lua_pushfstring(L, "%s.%s", LUAMONGO_ROOT, bson_name(bson_type));
	} else {
	    lua_pushstring(L, luaL_typename(L, 1));
	}	
    } else {
	lua_pushstring(L, luaL_typename(L, 1));
    }

    return 1;
}

/*
 * num = mongo.tonumber(obj) 
 */
static int bson_tonumber(lua_State *L) {
    int base = luaL_optint(L, 2, 10);
    if (base == 10) {  /* standard conversion */
	luaL_checkany(L, 1);
	if (lua_isnumber(L, 1)) {
	    lua_pushnumber(L, lua_tonumber(L, 1));
	    return 1;
	} else if (lua_istable(L, 1)) {
	    int bsontype_found = luaL_getmetafield(L, 1, "__bsontype");

	    if (bsontype_found) {
		lua_rawgeti(L, 1, 1);

		if (lua_isnumber(L, -1)) {
		    lua_pushnumber(L, lua_tonumber(L, -1));
		    return 1;
		}

		lua_pop(L, 1);
	    }
	}
    } else {
	const char *s1 = luaL_checkstring(L, 1);
	char *s2;
	unsigned long n;
	luaL_argcheck(L, 2 <= base && base <= 36, 2, "base out of range");
	n = strtoul(s1, &s2, base);
	if (s1 != s2) {  /* at least one valid digit? */
	    while (isspace((unsigned char)(*s2))) s2++;  /* skip trailing spaces */
		if (*s2 == '\0') {  /* no invalid trailing characters? */
		    lua_pushnumber(L, (lua_Number)n);
		    return 1;
		}
	    }
    }

    lua_pushnil(L);  /* else not a number */
    return 1;
}


int mongo_bsontypes_register(lua_State *L) {
    static const luaL_Reg bsontype_methods[] = {
        {"Date", bson_type_Date},
        {"Timestamp", bson_type_Timestamp},
        {"RegEx", bson_type_RegEx},
        {"NumberInt", bson_type_NumberInt},
        {"Symbol", bson_type_Symbol},
        {"type", bson_type_name},
        {"tonumber", bson_tonumber},
        {NULL, NULL}
    };

    luaL_register(L, LUAMONGO_ROOT, bsontype_methods);

    return 1;
}

