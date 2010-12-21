CFLAGS += -Wall -Wextra -g

all: sdp

sdp: sdp.o test.o

sdp.o test.o: sdp.h

clean:
	rm -f *.o tags

distclean: clean
	rm -f sdp
