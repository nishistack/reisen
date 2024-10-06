# $Id$

include $(PWD)/Platform/$(PLATFORM).mk

OBJS = main.o sfx.res

.PHONY: all clean
.SUFFIXES: .c .o .rc .res

all: reisen.sfx

reisen.sfx: $(OBJS)
	$(WINCC) -o $@ $(OBJS) $(WINLIBS)

.c.o:
	$(WINCC) -mwindows -I $(PWD)/zlib-include -c -o $@ $<

.rc.res:
	$(WINDRES) -O coff $< $@

clean:
	rm -f *.o *.sfx *.res
