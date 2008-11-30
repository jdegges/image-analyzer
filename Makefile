#CC=/home/jdegges/gumstix/gumstix-oe/tmp/cross/arm-angstrom-linux-gnueabi/bin/gcc
CC=gcc
CFLAGS=--fast-math -O6 -g -W -Wall
LIBS=-lm -lfreeimage -lpthread -lSDL
OBJECTS = analyze.o common.o iaio.o ia_sequence.o image_analyzer.o queue.o

ia: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o ia $(LIBS)

clean:
	rm *.o ia
