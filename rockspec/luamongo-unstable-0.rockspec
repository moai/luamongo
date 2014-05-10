package = "luamongo"
version = "unstable-0"

source = {
    url = "git://github.com/moai/luamongo.git",
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
   "lua >= 5.2"
}

external_dependencies = {
   LIBMONGOCLIENT = {
      header = "mongo/client/dbclient.h",
      library = "mongo-client",
   }
}

build = {
   type = "make",
   copy_directories = {},
   install_pass = false,
   install = { lib = { "mongo.so" } }
}
