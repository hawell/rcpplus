SRCS = test_preset.c test_h264_udp.c test_h263_udp.c test_h263_tcp.c \
test_detect.c test_coder.c test_ptz.c test_transcode.c test_play.c \
test_record.c test_jpeg.c test_audio.c test_alarm.c bulk_config.c

OBJS = $(SRCS:.c=../$(BUILD_DIR)/%.o)
INCS = -I../src/
LIB = ../$(BUILD_DIR)/$(LIBNAME) -ltlog-1.0.0 -lpthread
#FFMPEG = $(HOME)/Projects/ffmpeg/debug
SDLLIBS = -lSDL2
FFMPEGLIBS = -lavformat -lavcodec -lavutil -lswresample

ifdef FFMPEG
	FFMPEGLIBS += -lbz2 -llzma -lz
endif

ifeq ($(UNAME_S), Darwin)
ifdef FFMPEG
	FFMPEGLIBS += -liconv \
		-framework CoreFoundation -framework CoreMedia -framework CoreVideo \
		-framework AudioToolbox -framework VideoToolbox -framework Security
endif
else ifeq ($(UNAME_S), Linux)
    LIB += -ldispatch
	ifdef FFMPEG
		FFMPEGLIBS += -ldrm -lm -lva -lva-drm -lva-x11
		SDLLIBS += -lX11
	endif
endif

ifdef FFMPEG
	FFMPEGINC = -I$(FFMPEG)/include
	FFMPEGLDFLAGS = -L$(FFMPEG)/lib
endif

all: ../$(BUILD_DIR)/preset ../$(BUILD_DIR)/h264_udp ../$(BUILD_DIR)/h263_udp \
../$(BUILD_DIR)/h263_tcp ../$(BUILD_DIR)/detect ../$(BUILD_DIR)/coder \
../$(BUILD_DIR)/ptz ../$(BUILD_DIR)/transcode ../$(BUILD_DIR)/play \
../$(BUILD_DIR)/record ../$(BUILD_DIR)/jpeg ../$(BUILD_DIR)/audio \
../$(BUILD_DIR)/alarm ../$(BUILD_DIR)/config

../$(BUILD_DIR)/preset: ../$(BUILD_DIR)/test_preset.o
	@mkdir -p $(@D)
	$(CC) ../$(BUILD_DIR)/test_preset.o $(LIB) -o ../$(BUILD_DIR)/preset
	
../$(BUILD_DIR)/h264_udp: ../$(BUILD_DIR)/test_h264_udp.o
	@mkdir -p $(@D)
	$(CC) ../$(BUILD_DIR)/test_h264_udp.o $(LIB) -o ../$(BUILD_DIR)/h264_udp
	
../$(BUILD_DIR)/h263_udp: ../$(BUILD_DIR)/test_h263_udp.o
	@mkdir -p $(@D)
	$(CC) ../$(BUILD_DIR)/test_h263_udp.o $(LIB) -o ../$(BUILD_DIR)/h263_udp

../$(BUILD_DIR)/h263_tcp: ../$(BUILD_DIR)/test_h263_tcp.o
	@mkdir -p $(@D)
	$(CC) ../$(BUILD_DIR)/test_h263_tcp.o $(LIB) -o ../$(BUILD_DIR)/h263_tcp

../$(BUILD_DIR)/detect: ../$(BUILD_DIR)/test_detect.o
	@mkdir -p $(@D)
	$(CC) ../$(BUILD_DIR)/test_detect.o $(LIB) -o ../$(BUILD_DIR)/detect

../$(BUILD_DIR)/coder: ../$(BUILD_DIR)/test_coder.o
	@mkdir -p $(@D)
	$(CC) ../$(BUILD_DIR)/test_coder.o $(LIB) -o ../$(BUILD_DIR)/coder
	
../$(BUILD_DIR)/ptz: ../$(BUILD_DIR)/test_ptz.o
	@mkdir -p $(@D)
	$(CC) ../$(BUILD_DIR)/test_ptz.o $(LIB) -o ../$(BUILD_DIR)/ptz
	
../$(BUILD_DIR)/transcode: ../$(BUILD_DIR)/test_transcode.o
	@mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $(INCS) $(FFMPEGINC) test_transcode.c -o ../$(BUILD_DIR)/test_transcode.o
	$(CC) ../$(BUILD_DIR)/test_transcode.o $(LIB) $(FFMPEGLDFLAGS) $(FFMPEGLIBS) -o ../$(BUILD_DIR)/transcode
	
../$(BUILD_DIR)/play: ../$(BUILD_DIR)/test_play.o
	@mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $(INCS) $(FFMPEGINC) test_play.c -o ../$(BUILD_DIR)/test_play.o
	$(CC) ../$(BUILD_DIR)/test_play.o $(LIB) $(SDLLIBS) $(FFMPEGLDFLAGS) $(FFMPEGLIBS) -o ../$(BUILD_DIR)/play
	
../$(BUILD_DIR)/record: ../$(BUILD_DIR)/test_record.o
	@mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $(INCS) $(FFMPEGINC) test_record.c -o ../$(BUILD_DIR)/test_record.o
	$(CC) ../$(BUILD_DIR)/test_record.o $(LIB) $(SDLLIBS) $(FFMPEGLDFLAGS) $(FFMPEGLIBS) -o ../$(BUILD_DIR)/record

../$(BUILD_DIR)/jpeg: ../$(BUILD_DIR)/test_jpeg.o
	@mkdir -p $(@D)
	$(CC) ../$(BUILD_DIR)/test_jpeg.o $(LIB) -o ../$(BUILD_DIR)/jpeg

../$(BUILD_DIR)/audio: ../$(BUILD_DIR)/test_audio.o
	@mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $(INCS) $(FFMPEGINC) test_audio.c -o ../$(BUILD_DIR)/test_audio.o
	$(CC) ../$(BUILD_DIR)/test_audio.o $(LIB) $(SDLLIBS) $(FFMPEGLDFLAGS) $(FFMPEGLIBS) -o ../$(BUILD_DIR)/audio
	
../$(BUILD_DIR)/alarm: ../$(BUILD_DIR)/test_alarm.o
	@mkdir -p $(@D)
	$(CC) ../$(BUILD_DIR)/test_alarm.o $(LIB) -o ../$(BUILD_DIR)/alarm
	
../$(BUILD_DIR)/config: ../$(BUILD_DIR)/bulk_config.o
	@mkdir -p $(@D)
	$(CC) ../$(BUILD_DIR)/bulk_config.o $(LIB) -o ../$(BUILD_DIR)/config

../$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCS) $(FFMPEGINC) -c $< -o $@
	
clean:
	rm -f ../$(BUILD_DIR)/*.o  ../$(BUILD_DIR)/h264_udp h263_udp ../$(BUILD_DIR)/h263_tcp ../$(BUILD_DIR)/preset \
	../$(BUILD_DIR)/detect ../$(BUILD_DIR)/coder ../$(BUILD_DIR)/ptz ../$(BUILD_DIR)/transcode ../$(BUILD_DIR)/play \
	../$(BUILD_DIR)/record ../$(BUILD_DIR)/jpeg ../$(BUILD_DIR)/audio ../$(BUILD_DIR)/mpegts ../$(BUILD_DIR)/test

