debug = 0

ifneq ($(debug), 0)
    CFLAGS += -g -DDEBUG -D_DEBUG
    LDFLAGS += -g
endif

LDFLAGS += -lm

all: rbtree example

example: example1 example2

rbtree: rbtree.o test/asc16.o test/bitmap.o test/test.o
	$(CC) -o $@ $^ $(LDFLAGS)

example1: rbtree.o example/example1.o
	$(CC) -o $@ $^ $(LDFLAGS)

example2: rbtree.o example/example2.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean
clean:
	-rm -f *.o test/*.o example/*.o rbtree example1 example2


