BIN=blinky.c1d invaders.c1d mandel.c1d snake.c1d nick_scr0ll.c1d nick_w0rpcore.c1d
NIK=nick_scr0ll.n1k nick_w0rpcore.n1k nick_plain.n1k

TABLEFILES=jumptable.c jumptable.h usetable.h

l0dables: usetable.h $(BIN) $(NIK)

CFLAGS=-Wno-unused-variable -DRAD1O -DLPC43XX_M4
CFLAGS+=-mlong-calls # XXX: do we need those?

LDSCRIPT=../ld/l0dable.ld
RPATH=..
include ../Makefile.inc

$(TABLEFILES): EXPORTS mktable.pl
	./mktable.pl

clean::
	$(RM) $(TABLEFILES)
	$(RM) $(BIN:%.c1d=%.o) $(BIN:%.c1d=%.elf) $(BIN)
	$(RM) $(NIK:%.n1k=%.o) $(NIK:%.n1k=%.elf) $(NIK)
