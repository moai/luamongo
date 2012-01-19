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
extern void push_bsontype_table(lua_State* L, mongo::BSONType bsontype);
extern void lua_push_value(lua_State *L, const BSONElement &elem);

namespace {
    inline GridFile* userdata_to_gridfile(lua_State* L, int index) {
        void *ud = 0;

        ud = luaL_checkudata(L, index, LUAMONGO_GRIDFILE);
        GridFile *gridfile = *((GridFile **)ud);

        return gridfile;
    }
}

int gridfile_create(lua_State *L, GridFile gf) {
    GridFile **gridfile = (GridFile **)lua_newuserdata(L, sizeof(GridFile **));

    *gridfile = new GridFile(gf);

    luaL_getmetatable(L, LUAMONGO_GRIDFILE);
    lua_setmetatable(L, -2);

    return 1;
}

/*
 * chunk, err = gridfile:chunk(chunk_num)
 */
static int gridfile_chunk(lua_State *L) {
    GridFile *gridfile = userdata_to_gridfile(L, 1);
    int num = luaL_checkint(L, 2);
    int resultcount = 1;

    try {
        GridFSChunk c = gridfile->getChunk(num);
        GridFSChunk *chunk_ptr = new GridFSChunk(c);

        GridFSChunk **chunk = (GridFSChunk **)lua_newuserdata(L, sizeof(GridFSChunk *));
        *chunk = chunk_ptr;

        luaL_getmetatable(L, LUAMONGO_GRIDFSCHUNK);
        lua_setmetatable(L, -2);
    } catch (std::exception &e) {
        lua_pushnil(L);
        lua_pushfstring(L, LUAMONGO_ERR_GRIDFSCHUNK_FAILED, e.what());
        resultcount = 2;
    }

    return resultcount;
}

/*
 * chunk_size = gridfile:chunk_size()
 */
static int gridfile_chunk_size(lua_State *L) {
    GridFile *gridfile = userdata_to_gridfile(L, 1);

    lua_pushinteger(L, gridfile->getChunkSize());

    return 1;
}

/*
 * content_length = gridfile:content_length()
 * __len
 */
static int gridfile_content_length(lua_State *L) {
    GridFile *gridfile = userdata_to_gridfile(L, 1);

    lua_pushnumber(L, gridfile->getContentLength());

    return 1;
}

/*
 * bool = gridfile:exists()
 */
static int gridfile_exists(lua_State *L) {
    GridFile *gridfile = userdata_to_gridfile(L, 1);

    lua_pushboolean(L, gridfile->exists());

    return 1;
}

/*
 *
 */

static int gridfile_field(lua_State *L) {
    GridFile *gridfile = userdata_to_gridfile(L, 1);
    const char *field_name = luaL_checkstring(L, 2);

    BSONElement elem = gridfile->getFileField(field_name);
    lua_push_value(L, elem);

    return 1;
}

/*
 * filename_str = gridfile:filename()
 */
static int gridfile_filename(lua_State *L) {
    GridFile *gridfile = userdata_to_gridfile(L, 1);

    lua_pushstring(L, gridfile->getFilename().c_str());

    return 1;
}

/*
 * md5_str = gridfile:md5()
 */
static int gridfile_md5(lua_State *L) {
    GridFile *gridfile = userdata_to_gridfile(L, 1);

    lua_pushstring(L, gridfile->getMD5().c_str());

    return 1;
}

/*
 * metadata_table = gridfile:metadata()
 */
static int gridfile_metadata(lua_State *L) {
    GridFile *gridfile = userdata_to_gridfile(L, 1);

    bson_to_lua(L, gridfile->getMetadata());

    return 1;
}

/*
 * num_chunks = gridfile:num_chunks()
 */
static int gridfile_num_chunks(lua_State *L) {
    GridFile *gridfile = userdata_to_gridfile(L, 1);

    lua_pushinteger(L, gridfile->getNumChunks());

    return 1;
}

/*
 * date = gridfile:upload_date()
 */
static int gridfile_upload_date(lua_State *L) {
    GridFile *gridfile = userdata_to_gridfile(L, 1);

    Date_t upload_date = gridfile->getUploadDate();

    push_bsontype_table(L, mongo::Date);
    lua_pushnumber(L, upload_date);
    lua_rawseti(L, -2, 1);

    return 1;
}

/*
 * success,err = gridfile:write(filename)
 */
static int gridfile_write(lua_State *L) {
    GridFile *gridfile = userdata_to_gridfile(L, 1);
    const char *where = luaL_checkstring(L, 2);

    try {
        gridfile->write(where);
    } catch (std::exception &e) {
        lua_pushboolean(L, 0);
        lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_GRIDFILE, "write", e.what());
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}


/*
 * string = gridfile:data()
 */
static int gridfile_data(lua_State *L) {
    GridFile *gridfile = userdata_to_gridfile(L, 1);

    std::stringstream data(std::stringstream::out | std::stringstream::binary);
    try {
        gridfile->write(data);
    } catch (std::exception &e) {
        lua_pushboolean(L, 0);
        lua_pushfstring(L, LUAMONGO_ERR_CALLING, LUAMONGO_GRIDFILE, "data", e.what());
        return 2;
    }

    std::string data_str = data.str();
    lua_pushlstring (L, data_str.c_str(), data_str.length());
    return 1;
}

/*
 * __gc
 */
static int gridfile_gc(lua_State *L) {
    GridFile *gridfile = userdata_to_gridfile(L, 1);

    delete gridfile;

    return 0;
}

/*
 * __tostring
 */
static int gridfile_tostring(lua_State *L) {
    GridFile *gridfile = userdata_to_gridfile(L, 1);

    lua_pushfstring(L, "%s: %p", LUAMONGO_GRIDFILE, gridfile);

    return 1;
}

int mongo_gridfile_register(lua_State *L) {
    static const luaL_Reg gridfile_methods[] = {
        {"chunk", gridfile_chunk},
        {"chunk_size", gridfile_chunk_size},
        {"content_length", gridfile_content_length},
        {"exists", gridfile_exists},
        {"field", gridfile_field},
        {"filename", gridfile_filename},
        {"md5", gridfile_md5},
        {"metadata", gridfile_metadata},
        {"num_chunks", gridfile_num_chunks},
        {"upload_date", gridfile_upload_date},
        {"write", gridfile_write},
        {"data", gridfile_data},
        {NULL, NULL}
    };

    static const luaL_Reg gridfile_class_methods[] = {
        {NULL, NULL}
    };

    luaL_newmetatable(L, LUAMONGO_GRIDFILE);
    luaL_register(L, 0, gridfile_methods);
    lua_pushvalue(L,-1);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, gridfile_gc);
    lua_setfield(L, -2, "__gc");

    lua_pushcfunction(L, gridfile_tostring);
    lua_setfield(L, -2, "__tostring");

    lua_pushcfunction(L, gridfile_content_length);
    lua_setfield(L, -2, "__len");

    luaL_register(L, LUAMONGO_GRIDFILE, gridfile_class_methods);

    return 1;
}
