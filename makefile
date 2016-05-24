CC = gcc
CCFLAGS = -Wall -g

build: y86emul.c
	${CC} ${CCFLAGS} -o y86emul y86emul.c

clean:
	rm -rf *.o y86meul

