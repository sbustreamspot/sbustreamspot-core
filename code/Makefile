CC=g++
CFLAGS=-Wall --std=c++11
SOURCES := $(wildcard *.cpp)
OBJS := $(SOURCES:.cpp=.o)

.PHONY: clean

optimized: CFLAGS += -O3 -march=native -mtune=native
optimized: swoosh

debug: CFLAGS += -DDEBUG -g
debug: swoosh

swoosh: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

.cpp.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f swoosh $(OBJS)
