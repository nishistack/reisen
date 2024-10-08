# $Id$

CC = cc
WINCC = i686-w64-mingw32-gcc
WINDRES = i686-w64-mingw32-windres
CFLAGS = -std=c99 -g
LDFLAGS =
LIBS = -lz
BITS = 32
WINLIBS = $(PWD)/zlib$(BITS)/libz.a -lgdi32 -lshell32 -lcomctl32
