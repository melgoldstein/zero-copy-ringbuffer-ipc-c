CC = gcc
CFLAGS = -O3 -Wall -Wextra -Werror -std=c99 -Iinclude
all: throughput_bench
throughput_bench: src/spsc_ring.c benchmarks/throughput_bench.c include/spsc_ring.h
	$(CC) $(CFLAGS) src/spsc_ring.c benchmarks/throughput_bench.c -o $@
test:
	$(CC) $(CFLAGS) src/spsc_ring.c tests/test_ring.c -o test_ring
	./test_ring
ipc:
	$(CC) $(CFLAGS) src/spsc_ring.c tests/test_ipc.c -o test_ipc
	./test_ipc
clean:
	rm -f throughput_bench test_ring test_ipc *.exe
.PHONY: all test ipc clean
