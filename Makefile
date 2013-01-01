DESTDIR=/usr/local
MANDEST=share/man
CFLAGS=-Wall
LDFLAGS=-logg

all: opustags

opustags: opustags.c

man: opustags.1
	gzip <opustags.1 >opustags.1.gz

install: opustags man
	mkdir -p $(DESTDIR)/bin $(DESTDIR)/$(MANDEST)/man1
	install -m 755 opustags $(DESTDIR)/bin/
	install -m 644 opustags.1.gz $(DESTDIR)/$(MANDEST)/man1/

uninstall:
	rm -f $(DESTDIR)/bin/opustags
	rm -f $(DESTDIR)/$(MANDEST)/man1/opustags.1.gz

clean:
	rm -f opustags opustags.1.gz
