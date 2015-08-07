BIN=blinky.c1d invaders.c1d mandel.c1d nick_scr0ll.c1d

TABLEFILES=jumptable.c jumptable.h usetable.h

l0dables: usetable.h $(BIN)

CFLAGS=-Wno-unused-variable -DRAD1O -DLPC43XX_M4
CFLAGS+=-mlong-calls # XXX: do we need those?

LDSCRIPT=../ld/l0dable.ld
RPATH=..
include ../Makefile.inc

$(TABLEFILES): EXPORTS mktable.pl
	./mktable.pl

clean::
	$(RM) $(BIN:%.c1d=%.o) $(BIN:%.c0d=%.elf) $(BIN) $(TABLEFILES)
