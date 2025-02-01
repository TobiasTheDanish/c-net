SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)

CFLAGS = -Wall -Werror -ggdb
LFLAGS = -Isrc/include/

build: $(OBJECTS)
	@gcc $(OBJECTS) $(CFLAGS) -o c-net -lrt

%.o: %.c include/%.h
	@gcc -c $(CFLAGS) $< -o $@ -lrt
