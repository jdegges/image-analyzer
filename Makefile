#CC=/home/jdegges/gumstix/gumstix-oe/tmp/cross/arm-angstrom-linux-gnueabi/bin/gcc
CC=gcc
CFLAGS=--fast-math -O5 -pg
LIBS=-lm -lfreeimage -lpthread


ia: image_analyzer.o iaio.o ia_sequence.o analyze.o common.h
	${CC} ${CFLAGS} image_analyzer.o iaio.o ia_sequence.o analyze.o -o ia ${LIBS}

image_analyzer: image_analyzer.c image_analyzer.h common.h
	${CC} ${CFLAGS} -c image_analyzer.c -l ia_sequence.o

iaio: iaio.h iaio.c common.h
	${CC} ${CFLAGS} -c iaio.c

ias: ia_sequence.h ia_sequence.c common.h
	${CC} ${CFLAGS} -c ia_sequence.c

analyze: analyze.h analyze.c common.h
	${CC} ${CFLAGS} -c analyze.c

clean:
	rm image_analyzer.o iaio.o ia_sequence.o analyze.o ia
