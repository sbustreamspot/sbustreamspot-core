CC=g++
CFLAGS=-Wall --std=c++11 -O3 -march=native -mtune=native
SOURCES := swoosh.cpp
OBJS := $(SOURCES:.cpp=.o)

swoosh: swoosh.o
	$(CC) $(CFLAGS) $^ -o $@

.cpp.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f swoosh $(OBJS)
