package = "luamongo"
version = "scm-0"

source = {
  url = "https://github.com/moai/luamongo/archive/master.zip",
  dir = "luamongo-master",
}

description = {
  summary = "Lua client library for mongodb",
  detailed = [[
      luamongo: Lua mongo client library
   ]],
  homepage = "https://github.com/moai/luamongo",
  license = "MIT/X11",
}

dependencies = {
  "lua >= 5.2",
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
  install = { lib = { "mongo.so" } },
}
