SRC := writer.c
TARGET = writer
OBJS := $(SRC:.c=.o)
CFLAGS = -g -Wall 
CC = $(CROSS_COMPILE)gcc

all: writer1 writer2

writer1 : writer.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o writer.o writer.c

writer2 : writer.o
	$(CC) $(CFLAGS) $(INCLUDES) -I/ writer.o -o writer $(LDFLAGS)

clean:
	-rm -f *.o $(TARGET) *.elf *.map
