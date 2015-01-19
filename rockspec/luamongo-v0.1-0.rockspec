package = "luamongo"
version = "v0.1-0"

source = {
  url = "https://github.com/moai/luamongo/archive/v0.1.zip",
  dir = "luamongo-0.1",
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
  "lua == 5.1"
}

external_dependencies = {
  LIBMONGOCLIENT = {
    header = "mongo/client/dbclient.h",
    library = "mongoclient",
  },
  LIBSSL = {
    library = "ssl",
  },
  LIBBOOST_THREAD = {
    library = "boost_thread",
  },
  LIBBOOST_FILESYSTEM = {
    library = "boost_filesystem",
  },
}

build = {
  type = "make",
  copy_directories = {},
  install_pass = false,
  install = { lib = { "mongo.so" } }
}
