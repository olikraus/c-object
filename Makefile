
# CFLAGS = -g -DCO_USE_ZLIB -Wall -fsanitize=address -I./co
CFLAGS = -g -DCO_USE_ZLIB -Wall -I./co
# CFLAGS = -O4 -DCO_USE_ZLIB -Wall -I./co
LDFLAGS = -lz -lpthread


COSRC = $(shell ls ./co/*.c)
COOBJ = $(COSRC:.c=.o)

all: co_test co_a2l a2l_info
	
co_test: $(COOBJ) ./test/co_test.o
	$(CC) $(CFLAGS)  $^ -o $@ -lm $(LDFLAGS)


co_a2l: $(COOBJ) ./test/co_a2l.o
	$(CC) $(CFLAGS)  $^ -o $@ -lm $(LDFLAGS)

a2l_info: $(COOBJ) ./test/a2l_info.o
	$(CC) $(CFLAGS)  $^ -o $@ -lm $(LDFLAGS)

clean:
	-rm $(COOBJ) ./test/co_test.o ./test/co_a2l.o ./test/a2l_info.o co_test co_a2l a2l_info
	