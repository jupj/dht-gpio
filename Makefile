src = $(wildcard *.c)
obj = $(src:.c=.o)
bin = converter blink rtlog activate-dht

CC=gcc
CFLAGS=-O4

all: $(bin)

activate-dht: activate-dht.o convert.o
	$(CC) -o $@ $^ -lgpiod

blink: blink.o convert.o
	$(CC) -o $@ $^ -lgpiod

converter: converter.o convert.o
	$(CC) -o $@ $^

rtlog: rtlog.o convert.o
	$(CC) -o $@ $^ -lgpiod

tester: test.o convert.o
	$(CC) -o $@ $^

.PHONY: test
test: tester
	./tester
	rm -f tester

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
