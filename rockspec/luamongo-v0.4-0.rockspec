package = "luamongo"
version = "v0.4-0"

source = {
    url = "https://github.com/moai/luamongo.git",
    tag = "v0.4-0"
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
      library = "mongoclient",
   }
}

build = {
   type = "make",
   copy_directories = {},
   install_pass = false,
   install = { lib = { "mongo.so" } }
}
