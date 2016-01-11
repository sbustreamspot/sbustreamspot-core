CC=g++
CFLAGS=-Wall --std=c++11
SOURCES := swoosh.cpp
OBJS := $(SOURCES:.cpp=.o)

.PHONY: clean

optimized: CFLAGS += -O3 -march=native -mtune=native
optimized: swoosh

debug: CFLAGS += -DDEBUG -g
debug: swoosh

swoosh: swoosh.o
	$(CC) $(CFLAGS) $^ -o $@

.cpp.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f swoosh $(OBJS)
