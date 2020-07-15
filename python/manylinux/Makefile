
.PHONY: all
all: test

.PHONY: test
test: dist
	cd test && ./test.sh

dist:
	cd build && ./build.sh

.PHONY: clean
clean:
	rm -rf dist
