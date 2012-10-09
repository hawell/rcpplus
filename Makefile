NAME = rcpplus
MAJOR = 1
MINOR = 0
PATCH = 0
PREFIX = /usr/local
LIBNAME = lib$(NAME)-$(MAJOR).$(MINOR).$(PATCH).a
export LIBNAME
export PREFIX

.PHONY: src example

all:
	$(MAKE) -C src
	$(MAKE) -C example
	
clean:
	$(MAKE) -C src clean
	$(MAKE) -C example clean
	
install:
	$(MAKE) -C src install
	install -D -m 644 $(LIBNAME) $(PREFIX)/lib/$(LIBNAME) 