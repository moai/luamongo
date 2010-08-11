#define LUAMONGO_ROOT       "mongo"
#define LUAMONGO_CONNECTION "mongo.Connection"
#define LUAMONGO_CURSOR     "mongo.Cursor"
#define LUAMONGO_QUERY      "mongo.Query"
#define	LUAMONGO_GRIDFS		"mongo.GridFS"
#define	LUAMONGO_GRIDFILE	"mongo.GridFile"
#define	LUAMONGO_GRIDFSCHUNK	"mongo.GridFSChunk"

#define	LUAMONGO_ERR_CONNECTION_FAILED	"Connection failed: %s"
#define	LUAMONGO_ERR_GRIDFS_FAILED	"GridFS failed: %s"
#define	LUAMONGO_ERR_GRIDFSCHUNK_FAILED	"GridFSChunk failed: %s"
#define	LUAMONGO_ERR_QUERY_FAILED	"Query failed: %s"
#define	LUAMONGO_ERR_INSERT_FAILED	"Insert failed: %s"
#define	LUAMONGO_ERR_CONNECT_FAILED	"Connection to %s failed: %s"
#define	LUAMONGO_ERR_COUNT_FAILED	"Count failed: %s"
#define	LUAMONGO_ERR_REMOVE_FAILED	"Remove failed: %s"
#define	LUAMONGO_ERR_UPDATE_FAILED	"Update failed: %s"
#define	LUAMONGO_UNSUPPORTED_BSON_TYPE	"Unsupported BSON type `%s'"
#define	LUAMONGO_UNSUPPORTED_LUA_TYPE	"Unsupported Lua type `%s'"
#define	LUAMONGO_REQUIRES_JSON_OR_TABLE	"JSON string or Lua table required"
#define	LUAMONGO_REQUIRES_QUERY		LUAMONGO_QUERY ", JSON string or Lua table required"
#define	LUAMONGO_NOT_IMPLEMENTED	"Not implemented: %s.%s"
#define LUAMONGO_ERR_CALLING		"Error calling %s.%s: %s"

#if defined(MONGO_PRE_1_5)
    #define CHUNK Chunk
#else
    #define CHUNK GridFSChunk 
#endif
