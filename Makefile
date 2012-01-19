CC= g++
CFLAGS= -g -O2 -shared -fPIC -I /usr/include/lua5.1/ -I/usr/local/include/mongo/
AR= ar rcu
RANLIB= ranlib
RM= rm -f
LIBS=-lmongoclient -lboost_thread -lboost_filesystem
OUTLIB=mongo.so

LDFLAGS= $(LIBS)

OBJS = main.o mongo_bsontypes.o mongo_dbclient.o mongo_replicaset.o mongo_connection.o mongo_cursor.o mongo_gridfile.o mongo_gridfs.o mongo_gridfschunk.o mongo_query.o utils.o

all: luamongo

clean:
	$(RM) $(OBJS) $(OUTLIB)

luamongo: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(OUTLIB) $(LDFLAGS)

echo:
	@echo "CC = $(CC)"
	@echo "CFLAGS = $(CFLAGS)"
	@echo "AR = $(AR)"
	@echo "RANLIB = $(RANLIB)"
	@echo "RM = $(RM)"
	@echo "LDFLAGS = $(LDFLAGS)"

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

.PHONY: all 
