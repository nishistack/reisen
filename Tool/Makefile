# $Id$

include $(PWD)/Platform/$(PLATFORM).mk

OBJS = main.o sfx.o

.PHONY: all clean
.SUFFIXES: .c .o

all: reisen$(EXEC)

reisen$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

sfx.c: ../SFX/reisen.sfx
	xxd -i -n sfx ../SFX/reisen.sfx > $@

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.exe reisen sfx.c
