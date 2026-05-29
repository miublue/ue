CC = c89
CFLAGS = -DUSE_X11
PREFIX = /usr/local

all:
	${CC} -O2 ue.c -o ue -lncurses ${CFLAGS}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	install -s ue ${DESTDIR}${PREFIX}/bin

uninstall:
	rm ${DESTDIR}${PREFIX}/bin/ue
