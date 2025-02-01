SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)

VENDOR_PATH = vendor/build
VENDOR_OBJECTS = $(wildcard VENDOR_PATH/*.o)

CFLAGS = -Wall -Werror -ggdb
LFLAGS = -Isrc/include/

build: $(OBJECTS) $(VENDOR_OBJECTS)
	@gcc $(OBJECTS) $(VENDOR_OBJECTS) $(CFLAGS) -o c-net -lrt

%.o: %.c include/%.h
	@gcc -c $(CFLAGS) $< -o $@ -lrt

$(VENDOR_PATH)/coroutine.o: vendor/coroutines/coroutine.c
	@gcc -c $(CFLAGS) $< -o $@
