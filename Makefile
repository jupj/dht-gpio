all: converter

converter: converter.c
	gcc $< -o $@

.PHONY: test
test: converter
	./converter test

.PHONY: clean
clean:
	rm -f converter
