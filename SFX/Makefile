# $Id$

include $(PWD)/Platform/$(PLATFORM).mk

OBJS = main.o

.PHONY: all clean
.SUFFIXES: .c .o

all: reisen.sfx

reisen.sfx: $(OBJS)
	$(WINCC) -o $@ $(OBJS) $(WINLIBS)

.c.o:
	$(WINCC) -mwindows -I $(PWD)/zlib-include -c -o $@ $<

clean:
	rm -f *.o *.sfx
