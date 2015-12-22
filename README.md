# A Lua driver for mongodb

## Versions compatibility

| luamongo | mongo-c++-driver |
| -------- | ---------------- |
| v0.4.5   |   legacy-0.9.0   |
| v0.5.0   |   legacy-1.1.0   |

Version 1.0.0 is expected when the modern c++ driver is marked as stable. See
at the end of this file what changes are introduced from v0.4.5 to v0.5.0.

## Compilation

The makefile automatically detects which platform and Lua version are you
using, so for compilation you just need to do:

```
$ make
```

You can force the platform compilation by using `$ make Linux` or `$ make Darwin`.
Additionally, you can force the Lua version by doing:

```
$ make LUAPKG=lua5.2
```

where `lua5.2` can be replaced by `lua5.1` and `luajit`.


## Installation

Copy the library file `mongo.so` to any of the paths in LUA_CPATH environment
variable or Lua string `package.path`.

## Wiki Documentation

 * <a href="https://github.com/moai/luamongo/wiki/Bsontypes">BsonTypes</a>

 * <a href="https://github.com/moai/luamongo/wiki/Helperfunctions">HelperFunctions</a>

 * <a href="https://github.com/moai/luamongo/wiki/GridFS">GridFS</a>

 * <a href="https://github.com/moai/luamongo/wiki/GridFileBuilder">GridFileBuilder</a>

 * <a href="https://github.com/moai/luamongo/wiki/MongoConnection">MongoConnection</a> - START HERE

 * <a href="https://github.com/moai/luamongo/wiki/MongoReplicaSet">MongoReplicaSet</a>

 * <a href="https://github.com/moai/luamongo/wiki/MongoCursor">MongoCursor</a>

## Support

Submit issues to the <a
href="https://github.com/moai/luamongo/issues">moai github site</a>.

There is a <a href="http://groups.google.com/group/luamongo">Google
Groups mailing list</a>.

## Example

```Lua
local mongo = require('mongo')

-- Create a connection object
local db = assert(mongo.Connection.New())

-- connect to the server on localhost
assert(db:connect('localhost'))

-- insert a value into the namespace 'test.values'
assert(db:insert('test.values', {a = 10, b = 'str1'}))

-- the same using a JSON string
assert(db:insert('test.values', "{'a': 20, 'b': 'str2'}"))

-- insert a multiple values into the namespace 'test.values'
assert(db:insert_batch('test.values', {{a = 10, b = 'str1'}, {c = 11, d = 'str2'}}))

-- print the number of rows in the namespace 'test.values'
print(db:count('test.values'))

-- query all the values in the namespace 'test.values'
local q = assert(db:query('test.values', {}))

-- loop through the result set
for result in q:results() do
    print(result.a)
    print(result.b)
end
```

## How It Works

luamongo is a Lua library that wraps the <a
href="https://github.com/mongodb/mongo-cxx-driver">mongodb C++
API</a>. Currently it has been tested with
[legacy-0.9.0](https://github.com/mongodb/mongo-cxx-driver/tree/legacy-0.9.0)
version of this driver.

The current implementation does not give you raw access to the BSON
objects. BSON objects are passed to the API using a Lua table or a
JSON string representation. Every returned BSON document is fully
marshalled to a Lua table.

## Installing

luarocks can be used to install luamongo last SCM version:

    luarocks install "https://raw.githubusercontent.com/moai/luamongo/master/rockspec/luamongo-scm-0.rockspec"

or install any other of the versions available at `rockspec` directory.

For modern Linux systems, you will need to update your luarocks configuration
file, usually located at `/etc/luarocks/config.lua` or
`/usr/local/etc/luarocks/config.lua`, adding the following Lua table:

```Lua
external_deps_dirs = {
  {
    prefix='/usr/',
    include='include',
    lib='lib',
  },
  {
    prefix='/usr/',
    include='include',
    lib='lib/i386-linux-gnu',
  },
  {
    prefix='/usr/',
    include='include',
    lib='lib/x86_64-linux-gnu',
  },
  {
    prefix='/usr/local',
  },
}
```

## Changes from v0.4.5 to v0.5.0

- `GridFileBuilder` has been introduced into legacy C++ driver, luamongo don't
  implements it any more. The constructor has been changed and now it only
  receives an instance of GridFS class.

- `db:ensure_index()` function has been replaced by `db:create_index()`. The
  parameters of the new function are `create_index(ns,keys,options)` where
  `keys` and `options` can be Lua tables or JSON strings (both dictionaries
  which allow 'unique' and 'name' fields).

- `db:get_indexes()` is now `db:enumerate_indexes()`.

- `db:reset_index_cache()` has been removed.
