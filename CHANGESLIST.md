# Master version

# Version 0.4-beta

- Adapted to Lua 5.2: the major change in this version is the
compilation with Lua 5.2. Minor changes are needed in old scripts, in
order to follow the Lua 5.2 dont-use-global-variables statement.  So,
the `require` for the library **was**:

```Lua
require "mongo"
```

and now **it is**:

```Lua
local mongo = require "mongo"
```
