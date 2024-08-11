CC ?=
CFLAGS ?= -Wall -Wpedantic -Wextra -std=c11 -O2

.PHONY: all

all: writer reader

writer: ring_buffer/writer.c ring_buffer/ring_buffer.h ring_buffer/ring_buffer.c ring_buffer/message.h
	$(CC) $(CFLAGS) ring_buffer/writer.c ring_buffer/ring_buffer.c -o $@
	mv writer py

reader: ring_buffer/reader.c ring_buffer/ring_buffer.h ring_buffer/ring_buffer.c ring_buffer/message.h
	$(CC) $(CFLAGS) ring_buffer/reader.c ring_buffer/ring_buffer.c -o $@
	mv reader py

build_benchmark:
	g++ -o py/bench py/benchmark.cpp -O2

benchmark:    
	py/./bench

get_stat:
	python3.10 py/plot.py

clean:
	rm py/writer py/reader