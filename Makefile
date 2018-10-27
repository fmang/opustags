PNAME=opustags
DESTDIR=/usr/local
SRCDIR=src
DOCDIR=doc
MANDEST=share/man
DOCDEST=share/doc
CC=gcc
CFLAGS=-Wall
LDFLAGS=-logg -s
INSTALL=install
GZIP=gzip
RM=rm -f

all: $(PNAME) man

$(PNAME): $(SRCDIR)/$(PNAME).c $(SRCDIR)/picture.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

man: $(DOCDIR)/$(PNAME).1
	$(GZIP) < $^ > $(PNAME).1.gz

install: $(PNAME) man
	$(INSTALL) -d $(DESTDIR)/bin
	$(INSTALL) -m 755 $(PNAME) $(DESTDIR)/bin/
	$(INSTALL) -d $(DESTDIR)/$(MANDEST)/man1
	$(INSTALL) -m 644 $(PNAME).1.gz $(DESTDIR)/$(MANDEST)/man1/
	$(INSTALL) -d $(DESTDIR)/$(DOCDEST)/$(PNAME)
	$(INSTALL) -m 644 $(DOCDIR)/sample_audiobook.opustags $(DOCDIR)/CHANGELOG LICENSE README.md $(DESTDIR)/$(DOCDEST)/$(PNAME)

uninstall:
	$(RM) $(DESTDIR)/bin/$(PNAME)
	$(RM) $(DESTDIR)/$(MANDEST)/man1/$(PNAME).1.gz
	$(RM) -r $(DESTDIR)/$(DOCDEST)/$(PNAME)

clean:
	$(RM) $(PNAME) $(PNAME).1.gz
