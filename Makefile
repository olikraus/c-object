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

debug: CFLAGS = -g -DCO_USE_ZLIB -Wall -I./co -I./co/expat
sanitize: CFLAGS = -g -DCO_USE_ZLIB -Wall -fsanitize=address -I./co -I./co/expat
release: CFLAGS = -O4 -DNDEBUG -DCO_USE_ZLIB -Wall -I./co -I./co/expat
#gprof: CFLAGS = -g -pg -DCO_USE_ZLIB -Wall -I./co

ifeq ($(shell uname -s),Linux)
LDFLAGS = -lelf -lm -lz -lpthread
else
LDFLAGS = -Wl,-Bstatic -lelf -lm -lz -lpthread
endif

COSRC = ./co/co.c ./co/co_extra.c 
COOBJ = $(COSRC:.c=.o)
EXPATSRC = ./co/co_xml.c ./co/expat/xmlparse.c ./co/expat/xmlrole.c ./co/expat/xmltok.c 
EXPATOBJ = $(EXPATSRC:.c=.o)

debug: all

sanitize: all 

release: all
	strip a2l_info.exe
	strip a2l_search.exe
	
#gprof: all

all: co_test co_a2l a2l_info a2l_search csv2json csvprint hex2json elf2json json_search json_compare json_format outline xml_test
	
co_test: $(COOBJ) ./test/co_test.o
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

co_a2l: $(COOBJ) ./test/co_a2l.o
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

a2l_info: $(COOBJ) ./test/a2l_info.o
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS) 

a2l_search:  $(COOBJ) ./test/a2l_search.o
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

csv2json:  $(COOBJ) ./test/csv2json.o
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

csvprint:  $(COOBJ) ./test/csvprint.o
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

hex2json:  $(COOBJ) ./test/hex2json.o
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

elf2json:  $(COOBJ) ./test/elf2json.o
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

json_search:  $(COOBJ) ./test/json_search.o
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

json_compare:  $(COOBJ) ./test/json_compare.o
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

json_format:  $(COOBJ) ./test/json_format.o
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)
        
outline: $(COOBJ) $(EXPATOBJ) ./test/outline.o
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

xml_test: $(COOBJ) $(EXPATOBJ) ./test/xml_test.o
	$(CC) $(CFLAGS)  $^ -o $@ $(LDFLAGS)

clean:
	-rm $(COOBJ) 
	-rm $(EXPAT)
	-rm ./test/co_test.o ./test/co_a2l.o ./test/a2l_info.o ./test/a2l_search.o ./test/csv2json.o ./test/csvprint.o ./test/hex2json.o ./test/elf2json.o ./test/json_compare.o ./test/json_format.o ./test/outline.o ./test/xml_test.o
	-rm co_test co_a2l a2l_info csv2json csvprint hex2json elf2json json_search json_compare json_format outline xml_test
	