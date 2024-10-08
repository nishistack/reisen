# $Id$

CC = cc
WINCC = x86_64-w64-mingw32-gcc
WINDRES = x86_64-w64-mingw32-windres
CFLAGS = -std=c99 -g
LDFLAGS =
LIBS = -lz
BITS = 64
WINLIBS = $(PWD)/zlib$(BITS)/libz.a -lgdi32 -lshell32 -lcomctl32
