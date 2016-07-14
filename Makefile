CFLAGS+=
LDFLAGS+=-std=c++11 -lstdc++ -pthread -Wall -Wextra -lcurl

all:
	$(CC) *.cpp $(CFLAGS) $(LDFLAGS)

debug:
	$(CC) *.cpp $(CFLAGS) -O0 -g $(LDFLAGS)
