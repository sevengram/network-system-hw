CFLG=-O3 -Wall
LIBS=
CLEAN=rm -f client server *.o *.a

all: webServer

#  Compile and link
webServer:server.c strutil.c util.c http.c config.c
	gcc $(CFLG) -o $@ $^ $(LIBS)


#  Clean
clean:
	$(CLEAN)
