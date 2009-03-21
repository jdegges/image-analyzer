CC		:= gcc
CFLAGS	:= --fast-math -O6 -g -W -Wall
LIBS	:= -lm -lfreeimage -lpthread -lSDL
OBJECTS	:= $(patsubst %.c,%.o,$(wildcard src/*.c src/filters/*.c))
HEADERS := $(wildcard src/*.h src/filters/*.h)

ia : $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) $(OBJECTS) -o ia $(LIBS)

clean :
	rm -rf src/*.o src/filters/*.o ia
