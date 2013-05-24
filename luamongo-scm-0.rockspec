package = "luamongo"
version = "scm-0"

source = {
    url = "git://github.com/moai/luamongo.git"
}

description = {
   summary = "Lua client library for mongodb",
   detailed = [[
      luamongo: Lua mongo client library
   ]],
   homepage = "https://github.com/moai/luamongo",
   license = "MIT/X11"
}

dependencies = {
   "lua >= 5.1"
}

external_dependencies = {
   LIBMONGOCLIENT = {
      header = "mongo/client/dbclient.h",
      library = "libmongoclient.a",
   }
}

build = {
   type = "builtin",
   modules = {
      mongo = {
         sources = {
	    "main.cpp",
	    "mongo_bsontypes.cpp",
	    "mongo_connection.cpp",
	    "mongo_cursor.cpp",
	    "mongo_dbclient.cpp",
	    "mongo_gridfile.cpp",
	    "mongo_gridfs.cpp",
	    "mongo_gridfschunk.cpp",
	    "mongo_query.cpp",
	    "mongo_replicaset.cpp",
	    "utils.cpp"
         },
         libraries = {
            "mongoclient"
         },
         incdirs = {
	     "/usr/include/mongo"
	 }
      }
   }
}
