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
      library = "mongoclient",
   }
}

build = {
   type = "make",
   build_variables = {
    CC="g++",
    CFLAGS="-g -O2 -shared -fPIC -I/usr/include/mongo -I/usr/include/lua",
   },
   copy_directories = {},
   install_pass = false,
   install = { lib = { "mongo.so" } }
}
