SRC := aesdsocket.c
TARGET = aesdsocket
OBJS := $(SRC:.c=.o)
CFLAGS = -g -Wall -Werror
LDFLAGS = -lrt -pthread
CC = $(CROSS_COMPILE)gcc

all: aesdsocket.o aesdsocket

aesdsocket.o : aesdsocket.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o aesdsocket.o aesdsocket.c

aesdsocket : aesdsocket.o
	$(CC) $(CFLAGS) $(INCLUDES) -I/ aesdsocket.o -o aesdsocket $(LDFLAGS)

clean:
	-rm -f *.o $(TARGET) *.elf *.map
