NAME = rcpplus
MAJOR = 1
MINOR = 0
PATCH = 0
PREFIX = /usr/local
LIBNAME = lib$(NAME)-$(MAJOR).$(MINOR).$(PATCH).a
export LIBNAME

.PHONY: src example

all:
	$(MAKE) -C src
	$(MAKE) -C example
	
clean:
	$(MAKE) -C src clean
	$(MAKE) -C example clean
	
install:
	for i in `ls src/*.h`; do install -D -m 644 src/$i $(PREFIX)/include/rcpplus/$i; done
	install -D -m 644 $(LIBNAME) $(PREFIX)/lib/$(LIBNAME) 