CC=g++
CFLAGS=-Wall -g --std=c++11
SOURCES := $(wildcard *.cpp)
OBJS := $(SOURCES:.cpp=.o)

.PHONY: clean

optimized: CFLAGS += -Ofast -march=native -mtune=native
optimized: streamspot

debug: CFLAGS += -DDEBUG -g
debug: streamspot

streamspot: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

.cpp.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f streamspot $(OBJS)
