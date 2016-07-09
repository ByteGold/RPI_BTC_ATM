CFLAGS+=-g -O0
LDFLAGS+=-std=c++11 -lstdc++ -pthread

all:
	$(CC) *.cpp $(CFLAGS) $(LDFLAGS)
