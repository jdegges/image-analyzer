CC		:= gcc
CFLAGS	:= --fast-math -O6 -g -W -Wall
LIBS	:= -lm -lfreeimage -lpthread -lSDL
OBJECTS	:= $(patsubst %.c,%.o,$(wildcard *.c filters/*.c))
HEADERS := $(wildcard *.h filters/*.h)

ia : $(OBJECTS) $(HEADERS)
	$(CC) $(CFLAGS) $(OBJECTS) -o ia $(LIBS)

clean :
	rm *.o filters/*.o ia &> /dev/null || true
