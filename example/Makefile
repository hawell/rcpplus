CC = gcc
SRCS = test_preset.c test_h264_udp.c test_h263_udp.c test_detect.c test_coder.c test_ptz.c test_transcode.c
OBJS = $(SRCS:.c=.o)
INCS = -I../src/
CFLAGS = -g -std=c99 -Wall -W
LIB = ../$(LIBNAME)

FFMPEG = /home/arash/Programs/ffmpeg
FFMPEGLIB = \
		$(FFMPEG)/libavformat/libavformat.a \
		$(FFMPEG)/libavcodec/libavcodec.a \
		$(FFMPEG)/libavutil/libavutil.a \
		$(FFMPEG)/libswscale/libswscale.a \
		$(FFMPEG)/libavfilter/libavfilter.a -lz -lm -lpthread -lmp3lame -lfaac
		
FFMPEGINC = -I$(FFMPEG)

all: preset h264_udp h263_udp detect coder ptz transcode

preset: test_preset.o
	$(CC) test_preset.o $(LIB) -o preset
	
h264_udp: test_h264_udp.o
	$(CC) test_h264_udp.o $(LIB) -o h264_udp
	
h263_udp: test_h263_udp.o
	$(CC) test_h263_udp.o $(LIB) -o h263_udp

detect: test_detect.o
	$(CC) test_detect.o $(LIB) -o detect

coder: test_coder.o
	$(CC) test_coder.o $(LIB) -o coder
	
ptz: test_ptz.o
	$(CC) test_ptz.o $(LIB) -o ptz
	
transcode: test_transcode.o
	$(CC) test_transcode.o $(LIB) $(FFMPEGLIB) -o transcode
	
.c.o:
	$(CC) $(CFLAGS) $(INCS) $(FFMPEGINC) -c $< -o $@ 
	
clean:
	rm -f *.o  h264_udp h263_udp preset detect coder ptz transcode