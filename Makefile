# $Id$

PREFIX = /usr/local
PLATFORM = generic

include Platform/$(PLATFORM).mk

FLAGS = PLATFORM=$(PLATFORM) PWD=`pwd`

.PHONY: all install clean ./SFX ./Tool

all: ./SFX ./Tool

install: all
	mkdir -p $(PREFIX)/bin
	cp Tool/reisen $(PREFIX)/bin/

./SFX::
	$(MAKE) -C $@ $(FLAGS)

./Tool:: ./SFX
	$(MAKE) -C $@ $(FLAGS)

clean:
	$(MAKE) -C ./SFX clean $(FLAGS)
	$(MAKE) -C ./Tool clean $(FLAGS)
