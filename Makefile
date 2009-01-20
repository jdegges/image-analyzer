#CC		:= /home/jdegges/gumstix/gumstix-oe/tmp/cross/arm-angstrom-linux-gnueabi/bin/gcc
CC		:= gcc
CFLAGS	:= --fast-math -O6 -g -W -Wall
LIBS	:= -lm -lfreeimage -lpthread -lSDL
OBJECTS	:= $(patsubst %.c,%.o,$(wildcard *.c filters/*.c))

ia : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o ia $(LIBS)

clean :
	rm *.o filters/*.o ia &> /dev/null || true
