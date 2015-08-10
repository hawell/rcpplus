CC = gcc
SRCS = test_preset.c test_h264_udp.c test_h263_udp.c test_h263_tcp.c test_detect.c test_coder.c test_ptz.c test_transcode.c test_play.c test_record.c test_jpeg.c test_audio.c
OBJS = $(SRCS:.c=.o)
INCS = -I../src/
CFLAGS = -g -O0 -std=c99 -Wall -W -Wno-deprecated -Wno-deprecated-declarations
LIB = ../$(LIBNAME) -ltlog-1.0.0 -lpthread -lrt

FFMPEG = /home/arash/Projects/ffmpeg-2.0
FFMPEGLIB = \
		$(FFMPEG)/libavdevice/libavdevice.a \
		$(FFMPEG)/libavformat/libavformat.a \
		$(FFMPEG)/libavfilter/libavfilter.a \
		$(FFMPEG)/libavcodec/libavcodec.a \
		$(FFMPEG)/libswresample/libswresample.a \
		$(FFMPEG)/libswscale/libswscale.a \
		$(FFMPEG)/libavutil/libavutil.a \
		$(FFMPEG)/libpostproc/libpostproc.a \
		 -lz -lm -lpthread -lmp3lame -lfaac -lx264 -lbz2
		
FFMPEGINC = -I$(FFMPEG)

all: preset h264_udp h263_udp h263_tcp detect coder ptz transcode play record jpeg audio mpegts test

preset: test_preset.o
	$(CC) test_preset.o $(LIB) -o preset
	
h264_udp: test_h264_udp.o
	$(CC) test_h264_udp.o $(LIB) -o h264_udp
	
h263_udp: test_h263_udp.o
	$(CC) test_h263_udp.o $(LIB) -o h263_udp

h263_tcp: test_h263_tcp.o
	$(CC) test_h263_tcp.o $(LIB) -o h263_tcp

detect: test_detect.o
	$(CC) test_detect.o $(LIB) -o detect

coder: test_coder.o
	$(CC) test_coder.o $(LIB) -o coder
	
ptz: test_ptz.o
	$(CC) test_ptz.o $(LIB) -o ptz
	
transcode: test_transcode.o
	$(CC) test_transcode.o $(LIB) $(FFMPEGLIB) -o transcode
	
play: test_play.o
	$(CC) -c $(CFLAGS) $(INCS) $(FFMPEGINC) test_play.c -o test_play.o
	$(CC) test_play.o $(LIB) $(FFMPEGLIB) `sdl-config --cflags --libs` -o play
	
record: test_record.o
	$(CC) -c $(CFLAGS) $(INCS) $(FFMPEGINC) test_record.c -o test_record.o
	$(CC) test_record.o $(LIB) $(FFMPEGLIB) -o record

jpeg: test_jpeg.o
	$(CC) test_jpeg.o $(LIB) $(FFMPEGLIB) -o jpeg

audio: test_audio.o
	$(CC) test_audio.o $(LIB) $(FFMPEGLIB) `sdl-config --cflags --libs` -o audio
	
mpegts: mpegts.o
	$(CC) mpegts.o $(LIB) $(FFMPEGLIB) `sdl-config --cflags --libs` -o mpegts
	
test: test.o
	$(CC) test.o $(LIB) $(FFMPEGLIB) -o test
	
.c.o:
	$(CC) $(CFLAGS) $(INCS) $(FFMPEGINC) -c $< -o $@ 
	
clean:
	rm -f *.o  h264_udp h263_udp h263_tcp preset detect coder ptz transcode play record jpeg audio mpegts test