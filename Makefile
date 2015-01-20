CC= g++
LUAPKG:= $(shell ( luajit -e 'print("luajit")'  2> /dev/null ) || lua5.2 -e 'print("lua52")'  2> /dev/null || lua5.1 -e 'print("lua51")'  2> /dev/null || lua -e 'print("lua" .. string.match(_VERSION, "%d.%d"))'  2> /dev/null | cut -d' ' -f 2)
CFLAGS= -Wall -g -O2 -shared -fPIC -I/usr/include/mongo `pkg-config --cflags $(LUAPKG)` `pkg-config --cflags libmongo-client`
AR= ar rcu
RANLIB= ranlib
RM= rm -f
LIBS=`pkg-config --libs $(LUAPKG)` -lmongoclient -lssl -lboost_thread -lboost_filesystem
OUTLIB=mongo.so

LDFLAGS= $(LIBS)

OBJS = main.o mongo_bsontypes.o mongo_dbclient.o mongo_replicaset.o mongo_connection.o mongo_cursor.o mongo_gridfile.o mongo_gridfs.o mongo_gridfschunk.o mongo_query.o utils.o mongo_cxx_extension.o mongo_gridfilebuilder.o

UNAME = `uname`
PLAT = DetectOS

all: $(PLAT)

DetectOS:
	@make $(UNAME)

Linux:
	@make -f Makefile.linux

Darwin:
	@make -f Makefile.darwin

clean:
	$(RM) $(OBJS) $(OUTLIB)

.PHONY: all 
