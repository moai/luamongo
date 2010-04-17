CC= g++
CFLAGS= -g -O2 -shared -fpic -I /usr/include/lua5.1/ -I/usr/include/mongo/
AR= ar rcu
RANLIB= ranlib
RM= rm -f
LIBS=-lmongoclient -lboost_thread-mt -lboost_filesystem-mt
OUTLIB=mongo.so

LDFLAGS= $(LIBS)

OBJS = main.o mongo_connection.o mongo_cursor.o utils.o

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
mongo_connection.o: mongo_connection.cpp common.h utils.h
	$(CC) -c -o $@ $< $(CFLAGS)
mongo_cursor.o: mongo_cursor.cpp common.h utils.h
	$(CC) -c -o $@ $< $(CFLAGS)
utils.o: utils.cpp common.h utils.h
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: all 
