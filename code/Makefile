CC=g++
CFLAGS=-Wall -g --std=c++11
SOURCES := $(wildcard *.cpp)
OBJS := $(SOURCES:.cpp=.o)

.PHONY: clean

optimized: CFLAGS += -Ofast -march=native -mtune=native
optimized: swoosh

debug: CFLAGS += -DDEBUG -g
debug: swoosh

swoosh: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

.cpp.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f swoosh $(OBJS)
