CFLG=-O3 -Wall -pthread
LIBS=
CLEAN=rm -f webproxy *.o *.a

all: webproxy

#  Compile and link
webproxy:proxy.c log.c
	gcc $(CFLG) -o $@ $^ $(LIBS)


#  Clean
clean:
	$(CLEAN)
