# $Id$

include $(PWD)/Platform/$(PLATFORM).mk

OBJS = main.o sfx.o

.PHONY: all clean
.SUFFIXES: .c .o

all: reisen$(EXEC)

reisen$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

sfx.c: ../SFX/reisen.sfx
	xxd -i ../SFX/reisen.sfx | sed "s/___SFX_reisen_sfx/sfx/g" > $@

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.exe reisen sfx.c
