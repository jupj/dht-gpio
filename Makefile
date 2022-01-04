src = $(wildcard *.c)
obj = $(src:.c=.o)

CC=gcc
CFLAGS=-O4

all: converter

converter: converter.o convert.o
	$(CC) -o $@ $^ $(LDFLAGS)

tester: test.o convert.o
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: test
test: tester
	./tester
	rm -f tester

.PHONY: clean
clean:
	rm -f $(obj) converter
