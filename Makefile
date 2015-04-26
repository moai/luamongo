UNAME:= $(shell uname)
PLAT= DetectOS

CC= g++
LUAFLAGS= $(shell pkg-config --cflags $(LUAPKG))
LUAPKG:= $(shell ( luajit -e 'print("luajit")'  2> /dev/null ) || lua5.2 -e 'print("lua5.2")'  2> /dev/null || lua5.1 -e 'print("lua5.1")'  2> /dev/null || lua -e 'print("lua" .. string.match(_VERSION, "%d+.%d+"))'  2> /dev/null)
AR= ar rcu
RANLIB= ranlib
RM= rm -f
OUTLIB= mongo.so
OBJS = main.o mongo_bsontypes.o mongo_dbclient.o mongo_replicaset.o mongo_connection.o mongo_cursor.o mongo_gridfile.o mongo_gridfs.o mongo_gridfschunk.o mongo_query.o utils.o mongo_cxx_extension.o mongo_gridfilebuilder.o

# macports
ifneq ("$(wildcard /opt/local/include/mongo/client/dbclient.h)","")
LUA:= $(shell echo $(LUAPKG) | sed 's/[0-9].[0-9]//g')
VER:= $(shell echo $(LUAPKG) | sed 's/.*\([0-9].[0-9]\)/\1/g')
LUAFLAGS:= $(shell pkg-config --cflags '$(LUA) >= $(VER)')
MONGO_INCLUDE_DIR= /opt/local/include/mongo/
MONGO_LIB_DIR= /opt/local/lib
CFLAGS:= -Wall -g -O2 -fPIC $(LUAFLAGS) -I$(MONGO_INCLUDE_DIR)
LIBS:= $(shell pkg-config --libs "$(LUA) >= $(VER)") -lmongoclient -lssl -lboost_thread-mt -lboost_filesystem-mt -flat_namespace -bundle -L$(MONGO_LIB_DIR) -rdynamic
endif

# homebrew
ifneq ("$(wildcard /usr/local/include/mongo/client/dbclient.h)","")
LUA:= $(shell echo $(LUAPKG) | sed 's/[0-9].[0-9]//g')
VER:= $(shell echo $(LUAPKG) | sed 's/.*\([0-9].[0-9]\)/\1/g')
LUAFLAGS:= $(shell pkg-config --cflags '$(LUA) >= $(VER)')
MONGO_INCLUDE_DIR= /usr/local/include/mongo/
MONGO_LIB_DIR= /usr/local/lib
CFLAGS:= -Wall -g -O2 -fPIC $(LUAFLAGS) -I$(MONGO_INCLUDE_DIR)
LIBS:= $(shell pkg-config --libs "$(LUA) >= $(VER)") -lmongoclient -lssl -lboost_thread-mt -lboost_filesystem-mt -flat_namespace -bundle -L$(MONGO_LIB_DIR) -rdynamic
endif

ifeq ("$(LIBS)", "")
MONGOFLAGS:= $(shell pkg-config --cflags libmongo-client)
CFLAGS:= -Wall -g -O2 -shared -fPIC -I/usr/include/mongo $(LUAFLAGS) $(MONGOFLAGS)
LIBS:= $(shell pkg-config --libs $(LUAPKG)) -lmongoclient -lssl -lboost_thread -lboost_filesystem -lrt
endif

LDFLAGS:= $(LIBS)

all: check $(PLAT)

DetectOS: check
	@make $(UNAME)

Linux: luamongo

Darwin: checkdarwin luamongo

check:
	@if [ -z $(LUAPKG) ]; then echo "Impossible to detect Lua version, you need LuaJIT, Lua 5.1 or Lua 5.2 installed!"; exit 1; fi
	@if [ -z $(LUAFLAGS) ]; then echo "Unable to configure with pkg-config, luamongo needs developer version of $(LUAPKG). You can force other Lua version by declaring variable LUAPKG=lua5.1 or LUAPKG=lua5.2"; exit 1; fi

checkdarwin:
ifeq ("$(MONGO_LIB_DIR)", "")
	@echo "To build luamongo on Darwin, you must have either ports or homebrew (preferred) installed!"
	exit 1
endif

echo: check
	@echo "CC = $(CC)"
	@echo "CFLAGS = $(CFLAGS)"
	@echo "AR = $(AR)"
	@echo "RANLIB = $(RANLIB)"
	@echo "RM = $(RM)"
	@echo "LDFLAGS = $(LDFLAGS)"

clean:
	$(RM) $(OBJS) $(OUTLIB)

############################################################################~


luamongo: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(OUTLIB) $(LDFLAGS)

main.o: main.cpp utils.h
	$(CC) -c -o $@ $< $(CFLAGS)
mongo_dbclient.o: mongo_dbclient.cpp common.h utils.h
	$(CC) -c -o $@ $< $(CFLAGS)
mongo_connection.o: mongo_connection.cpp common.h utils.h
	$(CC) -c -o $@ $< $(CFLAGS)
mongo_cursor.o: mongo_cursor.cpp common.h utils.h
	$(CC) -c -o $@ $< $(CFLAGS)
mongo_gridfile.o: mongo_gridfile.cpp common.h utils.h
	$(CC) -c -o $@ $< $(CFLAGS)
mongo_gridfs.o: mongo_gridfs.cpp common.h utils.h
	$(CC) -c -o $@ $< $(CFLAGS)
mongo_gridfschunk.o: mongo_gridfschunk.cpp common.h utils.h
	$(CC) -c -o $@ $< $(CFLAGS)
mongo_query.o: mongo_query.cpp common.h utils.h
	$(CC) -c -o $@ $< $(CFLAGS)
mongo_replicaset.o: mongo_replicaset.cpp common.h utils.h
	$(CC) -c -o $@ $< $(CFLAGS)
mongo_bsontypes.o: mongo_bsontypes.cpp common.h
	$(CC) -c -o $@ $< $(CFLAGS)
utils.o: utils.cpp common.h utils.h
	$(CC) -c -o $@ $< $(CFLAGS)
mongo_cxx_extension.o: mongo_cxx_extension.cpp mongo_cxx_extension.h
	$(CC) -c -o $@ $< $(CFLAGS)
mongo_gridfilebuilder.o: mongo_gridfilebuilder.cpp mongo_cxx_extension.h common.h utils.h
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: all check checkdarwin clean DetectOS Linux Darwin echo
