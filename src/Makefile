CC = gcc
SRCS = rcpplus.c rcpcommand.c md5.c rtp.c preset.c device.c coder.c ptz.c audio.c
HDRS = rcpplus.h rcpcommand.h md5.h rtp.h preset.h device.h coder.h ptz.h rcpdefs.h audio.h
OBJS = $(SRCS:.c=.o)
CFLAGS = -g -std=c99 -Wall -W

all : $(OBJS)
	ar  rcf ../$(LIBNAME) $(OBJS)
	
%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@
	
install:
	mkdir -p $(PREFIX)/include/rcpplus/
	install -m 644 -t $(PREFIX)/include/rcpplus/ $(HDRS)
	
clean:
	rm -f *.o ../*.a