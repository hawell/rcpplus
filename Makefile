CONFIG ?= debug
CC ?= gcc

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S), Darwin)
    CC = clang
endif

ifeq ($(CONFIG), debug)
    CFLAGS = -g -O0 -DDEBUG -std=c99 -Wall -W -Wno-deprecated -Wno-deprecated-declarations
    BUILD_DIR = debug
    PREFIX = /tmp
else ifeq ($(CONFIG), release)
    CFLAGS = -O2 -DNDEBUG -Wall -std=c99 -Wall -W -Wno-deprecated -Wno-deprecated-declarations
    BUILD_DIR = release
    PREFIX = /usr/local
else
    $(error Unknown CONFIG '$(CONFIG)'. Use either 'debug' or 'release')
endif

NAME = rcpplus
MAJOR = 1
MINOR = 0
PATCH = 0
LIBNAME = lib$(NAME)-$(MAJOR).$(MINOR).$(PATCH).a

export UNAME_S
export LIBNAME
export PREFIX
export BUILD_DIR
export CC
export CFLAGS

.PHONY: src example

all: src example

src:
	$(MAKE) -C src
	
example: $(BUILD_DIR)/$(LIBNAME)
	$(MAKE) -C example
	
clean:
	$(MAKE) -C src clean
	$(MAKE) -C example clean
	
install:
	$(MAKE) -C src install
	install -D -m 644 $(BUILD_DIR)/$(LIBNAME) $(PREFIX)/lib/$(LIBNAME)
