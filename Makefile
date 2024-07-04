PROG=aptinfo
OBJS=main.o
PREFIX=/usr/local/bin

%.o: %.c
	gcc -pedantic -Wall -c -o $@ $<

${PROG}: ${OBJS}
	gcc -o $@ $<

install:
	cp ${PROG} ${PREFIX}/${PROG}

clean:
	rm -f ${PROG} ${OBJS}
