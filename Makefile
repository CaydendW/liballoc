CFLAGS=-O3 -std=gnu99 -Wall -Wextra -Werror -fPIC
HEADERPATH=-I./

all: clean unix

clean:
	rm -f ./*.o
	rm -f ./*.a
	rm -f ./*.so

compile:
	$(CC) $(HEADERPATH) $(CFLAGS) -static -c liballoc.c
	$(AR) -rcv liballoc.a  *.o
	$(CC) $(HEADERPATH) $(CFLAGS) -shared liballoc.c -o liballoc.so

unix:
	$(CC) $(HEADERPATH) $(CFLAGS) -static -c liballoc.c liballoc_unix.c
	$(AR) -rcv liballoc.a  *.o
	$(CC) $(HEADERPATH) $(CFLAGS) -shared liballoc.c liballoc_unix.c -o liballoc.so
