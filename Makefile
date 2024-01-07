
CFLAGS = -g -DCO_USE_ZLIB -Wall -fsanitize=address -I./co
LDFLAGS = -lz


COSRC = $(shell ls ./co/*.c)
COOBJ = $(COSRC:.c=.o)

all: co_test co_a2l
	
co_test: $(COOBJ) ./test/co_test.o
	$(CC) $(CFLAGS)  $^ -o $@ -lm $(LDFLAGS)


co_a2l: $(COOBJ) ./test/co_a2l.o
	$(CC) $(CFLAGS)  $^ -o $@ -lm $(LDFLAGS)

clean:
	-rm $(COOBJ) co_test co_a2l
	