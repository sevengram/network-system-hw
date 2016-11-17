CFLG=-O3 -Wall -pthread
LIBS=-lssl -lcrypto
CLEAN=rm -f client dfc *.o *.a

all:client dfc

#  Compile and link
client:client.c util.c strutil.c transmission.c config.c
	gcc $(CFLG) -o $@ $^ $(LIBS)

dfc:server.c util.c strutil.c transmission.c config.c
	gcc $(CFLG) -o $@ $^ $(LIBS)


#  Clean
clean:
	$(CLEAN)
