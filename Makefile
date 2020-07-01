# popcorn - Suckless hot key daemon
# See LICENSE file for copyright and license details.

include config.mk

SRC = popcorn.c
OBJ = ${SRC:.c=.o}

all: clean options popcorn

options:
	@echo popcorn build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	${CC} -c ${CFLAGS} $<

config.h:
	cp config.def.h config.h

popcorn: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f popcorn ${OBJ}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f popcorn ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/popcorn
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < popcorn.1 > ${DESTDIR}${MANPREFIX}/man1/popcorn.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/popcorn.1

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/popcorn\
		${DESTDIR}${MANPREFIX}/man1/popcorn.1

.PHONY: all options clean dist install uninstall
