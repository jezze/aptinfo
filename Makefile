BIN=aptinfo
OBJS=main.o
PREFIX=/usr/local
CC=gcc
CFLAGS=-pedantic -Wall -c
LD=gcc
LDFLAGS=
CP=cp
RM=rm

.PHONY: all debug install clean

all: ${BIN}
debug: CFLAGS+=-g
debug: ${BIN}

%.o: %.c
	@echo CC $@
	@${CC} ${CFLAGS} -o $@ $<

${BIN}: ${OBJS}
	@echo LD $@
	@${LD} ${LDFLAGS} -o $@ $^

install:
	${CP} ${BIN} ${PREFIX}/bin/${BIN}

clean:
	${RM} -f ${BIN} ${OBJS}
