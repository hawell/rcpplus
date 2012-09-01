NAME = rcpplus
VERSION = 1.0.0
CC = gcc
SRCS = rcpplus.c rcpcommand.c md5.c rtp.c
OBJS = $(SRCS:.c=.o)
CFLAGS = -g -std=c99 -Wall -W

all : $(OBJS)
	ar  rcf $(NAME)-$(VERSION).a $(OBJS)
	
test: test.c $(OBJS)
	$(CC) $(CFLAGS) -c test.c -o test.o
	$(CC) $(OBJS) test.o -o test

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@ 
	
clean:
	rm -f *.o *.a test