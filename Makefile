#
#  c-object Makefile
#  (c) 2024 Oliver Kraus
#  https://github.com/olikraus/c-object
#  CC BY-SA 3.0  https://creativecommons.org/licenses/by-sa/3.0/
#
#  targets
#	clean		clear all objects and executables
#	debug		build debug version (default)
#	sanitize	build debug version with gcc sanitize enabled
#	release		build release version
#

debug: CFLAGS = -g -DCO_USE_ZLIB -Wall -I./co 
sanitize: CFLAGS = -g -DCO_USE_ZLIB -Wall -fsanitize=address -I./co
release: CFLAGS = -O4 -DNDEBUG -DCO_USE_ZLIB -Wall -I./co
#gprof: CFLAGS = -g -pg -DCO_USE_ZLIB -Wall -I./co

ifeq ($(shell uname -s),Linux)
LDFLAGS = -lm -lz -lpthread
else
LDFLAGS = -Wl,-Bstatic -lm -lz -lpthread
endif

COSRC = $(shell ls ./co/*.c)
COOBJ = $(COSRC:.c=.o)

debug: all

sanitize: all 

release: all
	strip a2l_info.exe
	strip a2l_search.exe
	
#gprof: all

all: co_test co_a2l a2l_info a2l_search
	
co_test: $(COOBJ) ./test/co_test.o
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

co_a2l: $(COOBJ) ./test/co_a2l.o
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

a2l_info: $(COOBJ) ./test/a2l_info.o
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS) 

a2l_search:  $(COOBJ) ./test/a2l_search.o
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

clean:
	-rm $(COOBJ) ./test/co_test.o ./test/co_a2l.o ./test/a2l_info.o ./test/a2l_search.o co_test co_a2l a2l_info
	