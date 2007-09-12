CC= cgcc
CFLAGS= -Wall -O2 -g
LDFLAGS=
MODULES= main.o decrunch.o ppdepack.o mmcmp.o s404_dec.o unsqsh.o
RELDIR=amigadepacker-0.02

amigadepacker:	$(MODULES)
	$(CC) -o $@ $(MODULES)

main.o:	main.c decrunch.h version.h
	$(CC) $(CFLAGS) -c $<

decrunch.o:	decrunch.c decrunch.h ppdepack.h
	$(CC) $(CFLAGS) -c $<

ppdepack.o:	ppdepack.c ppdepack.h decrunch.h
	$(CC) $(CFLAGS) -c $<

mmcmp.o:	mmcmp.c mmcmp.h decrunch.h
	$(CC) $(CFLAGS) -c $<

s404_dec.o:	s404_dec.c s404_dec.h decrunch.h
	$(CC) $(CFLAGS) -c $<

unsqsh.o:	unsqsh.c unsqsh.h decrunch.h
	$(CC) $(CFLAGS) -c $<

install:	amigadepacker
	mkdir -p /usr/local/bin
	install amigadepacker /usr/local/bin/
	mkdir -p /usr/local/share/man/man1
	install amigadepacker.1 /usr/local/share/man/man1/

release:	
	rm -rf $(RELDIR)
	mkdir -p $(RELDIR)/tests
	cp COPYING COPYING.GPL ChangeLog Makefile.in README *.1 configure *.c *.h $(RELDIR)/
	cp tests/check.sh tests/pp20* tests/sqsh* tests/aon.* $(RELDIR)/tests/
	rm -f $(RELDIR)/version.h

check:	amigadepacker
	cd tests && ./check.sh

clean:	
	rm -f *.o
