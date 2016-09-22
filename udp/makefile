CFLG=-O3 -Wall
LIBS=
CLEAN=rm -f client server *.o *.a

all:client server

#  Compile and link
client:client.c util.c
	gcc $(CFLG) -o $@ $^ $(LIBS)

server:server.c util.c
	gcc $(CFLG) -o $@ $^ $(LIBS)


#  Clean
clean:
	$(CLEAN)
