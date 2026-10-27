CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2

.PHONY: test test-fs clean

test: test-fs

test-fs: build/fs_test
	./build/fs_test

build/fs_test: kernel/fs.c kernel/fs.h tests/fs_test.c
	mkdir -p build
	$(CC) $(CFLAGS) -Ikernel kernel/fs.c tests/fs_test.c -o $@

clean:
	rm -rf build
