CLANG = clang

INCLUDE_PATH = -I/usr/share/liburing/src/include
LIBRARY_PATH = -L/usr/lib/x86_64-linux-gnu
LIB = -luring -lrt -laio

.PHONY: clean 

clean:
	rm -f Benchmark

test:	iouring_read_write.c
	clang -o Benchmark $(INCLUDE_PATH) $(LIBRARY_PATH) $(LIB) $?
	
build: test
run:
	sudo ./Benchmark uring in.txt out.txt & sleep 1 && sudo python3 test.py --iotype uring
.DEFAULT_GOAL := build