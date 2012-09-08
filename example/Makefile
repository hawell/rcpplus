CC = gcc
SRCS = test_preset.c test_h264_udp.c test_h263_udp.c
OBJS = $(SRCS:.c=.o)
INCS = -I../src/
CFLAGS = -g -std=c99 -Wall -W
LIB = ../$(LIBNAME)

all: preset h264_udp h263_upd

preset: test_preset.o
	$(CC) test_preset.o $(LIB) -o preset
	
h264_udp: test_h264_udp.o
	$(CC) test_h264_udp.o $(LIB) -o h264_udp
	
h263_upd: test_h263_udp.o
	$(CC) test_h263_udp.o $(LIB) -o h263_udp

.c.o:
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@ 
	
clean:
	rm -f *.o  h264_udp h263_udp preset