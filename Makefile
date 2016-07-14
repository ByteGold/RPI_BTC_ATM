CFLAGS+=-g -O0
LDFLAGS+=-std=c++11 -lstdc++ -pthread -Wall -Wextra -lcurl

all:
	$(CC) *.cpp $(CFLAGS) $(LDFLAGS)
