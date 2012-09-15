CC = gcc
SRCS = rcpplus.c rcpcommand.c md5.c rtp.c rcplog.c preset.c device.c coder.c ptz.c
OBJS = $(SRCS:.c=.o)
CFLAGS = -g -std=c99 -Wall -W

all : $(OBJS)
	ar  rcf ../$(LIBNAME) $(OBJS)
	
%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@
	
clean:
	rm -f *.o ../*.a