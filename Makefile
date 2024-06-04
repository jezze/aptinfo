PROG=aptinfo
OBJS=main.o

%.o: %.c
	gcc -pedantic -Wall -c -o $@ $<

${PROG}: ${OBJS}
	gcc -o $@ $<

clean:
	rm -f ${PROG} ${OBJS}
