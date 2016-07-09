CFLAGS+=-g -O0
LDFLAGS+=-std=c++11 -lstdc++ -pthread -Wall -Wextra

all:
	$(CC) *.cpp $(CFLAGS) $(LDFLAGS)
