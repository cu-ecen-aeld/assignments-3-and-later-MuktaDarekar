ifeq ($(CC),)
	CC = $(CROSS_COMPILE)gcc
endif

ifeq ($(CFLAGS),)
	CFLAGS = -g -Wall -Werror 
endif

ifeq ($(LDFLAGS),)
	LDFLAGS = -pthread -lrt
endif

all: aesdsocket.o aesdsocket

default: aesdsocket.o aesdsocket

aesdsocket.o : aesdsocket.c
	$(CC) $(CFLAGS) -c -o aesdsocket.o aesdsocket.c $(INCLUDES)

aesdsocket : aesdsocket.o
	$(CC) $(CFLAGS) aesdsocket.o -o aesdsocket $(INCLUDES) $(LDFLAGS)

clean:
	-rm -f *.o $(TARGET) *.elf *.map
