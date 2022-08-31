# This is the default target, which will be built when 
# you invoke make
.PHONY: all
all: statx-test

# This rule tells make how to build hello from hello.cpp
statx-test: statx-test.c
	gcc -o statx-test statx-test.c

.PHONY: test
test:
	./statx-test

# This rule tells make to copy hello to the binaries subdirectory,
# creating it if necessary
.PHONY: install
install:
	mkdir -p bin
	cp -p statx-test bin

# This rule tells make to delete hello and hello.o
.PHONY: clean 
clean:
	rm -f statx-test statx-test.o
