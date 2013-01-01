CC=clang
CFLAGS=-Wall
LDFLAGS=-logg

all: opustags

opustags: opustags.c

clean:
	rm -f opustags
