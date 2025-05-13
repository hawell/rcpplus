SRCS = rcpplus.c rcpcommand.c md5.c rtp.c preset.c device.c coder.c caps.c ptz.c audio.c alarm.c
HDRS = rcpplus.h rcpcommand.h md5.h rtp.h preset.h device.h coder.h caps.h ptz.h rcpdefs.h audio.h alarm.h
OBJS = $(patsubst %.c, ../$(BUILD_DIR)/%.o, $(SRCS))

all : $(OBJS)
	ar rc ../$(BUILD_DIR)/$(LIBNAME) $(OBJS)

../$(BUILD_DIR)/%.o: %.c %.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
	
install:
	mkdir -p $(PREFIX)/include/rcpplus/
	install -m 644 $(HDRS) $(PREFIX)/include/rcpplus/
	
clean:
	rm -f *.o ../*.a
