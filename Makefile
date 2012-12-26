prefix=/usr/local
bindir=${prefix}/bin
DESTDIR=

all:	mlog

mlog: mlog.c
	gcc -Wall -Wpointer-arith -Wstrict-prototypes -O2 -o mlog mlog.c -lcurses

install: mlog
	install -d $(DESTDIR)${bindir}
	install -m 755 -o root -g wheel mlog $(DESTDIR)${bindir}

uninstall:
	rm $(DESTDIR)${bindir}/mlog

clean:
	rm mlog mlog-*.tar.gz* || true
