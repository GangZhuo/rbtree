debug = 0

ifneq ($(debug), 0)
    CFLAGS += -g -DDEBUG -D_DEBUG
    LDFLAGS += -g
endif

LDFLAGS += -lm

all: rbtree

rbtree: rbtree.o test/asc16.o test/bitmap.o test/test.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean
clean:
	-rm -f *.o test/*.o rbtree


