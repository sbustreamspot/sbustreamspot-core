CC=g++
CFLAGS=-Wall --std=c++11 -Ofast -march=native
TARGET := swoosh
OBJS := swoosh.o

$(TARGET): $(OBJS)
	    $(CC) $(CFLAGS) $^ -o $@

clean:
	    rm -f $(TARGET) $(OBJS)
