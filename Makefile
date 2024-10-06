# $Id$

PLATFORM = generic

include Platform/$(PLATFORM).mk

FLAGS = PLATFORM=$(PLATFORM) PWD=`pwd`

.PHONY: all clean ./SFX ./Tool

all: ./SFX ./Tool

./SFX::
	$(MAKE) -C $@ $(FLAGS)

./Tool:: ./SFX
	$(MAKE) -C $@ $(FLAGS)

clean:
	$(MAKE) -C ./SFX clean $(FLAGS)
	$(MAKE) -C ./Tool clean $(FLAGS)
