CC=g++
CFLAGS=-Wall --std=c++11 -Ofast -march=native -mtune=native -funsafe-loop-optimizations -ffast-math -Ofast 
SOURCES := swoosh.cpp
OBJS := $(SOURCES:.cpp=.o)

swoosh: swoosh.o
	$(CC) $(CFLAGS) $^ -o $@

.cpp.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f swoosh $(OBJS)
