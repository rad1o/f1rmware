C1D=cube.c1d blinky.c1d invaders.c1d mandel.c1d snake.c1d snake2.c1d bricks.c1d schedule.c1d ws2812b.c1d battery.c1d fire.c1d colorp.c1d tetris.c1d sysinfo.c1d colors.c1d

N1K=nick_scr0ll.n1k nick_w0rpcore.n1k nick_matrix.n1k nick_plain.n1k nick_invaders.n1k nick_anim.n1k nick_image.n1k nick_netz39.n1k nick_life.n1k nick_colplasm.n1k nick_ledbow.n1k

TABLEFILES=jumptable.c jumptable.h usetable.h

l0dables: usetable.h $(C1D) $(N1K)

CFLAGS=-Wno-unused-variable -DRAD1O -DLPC43XX_M4 -DL0DABLE
CFLAGS+=-mlong-calls # XXX: do we need those?

LDSCRIPT=../ld/l0dable.ld
RPATH=..
include ../Makefile.inc

$(TABLEFILES): EXPORTS mktable.pl
	./mktable.pl

clean::
	$(RM) $(TABLEFILES)
	$(RM) -f *.n1k *.c1d *.o *.elf
#	$(RM) $(C1D:%.c1d=%.o) $(C1D:%.c1d=%.elf) $(C1D)
#	$(RM) $(N1K:%.n1k=%.o) $(N1K:%.n1k=%.elf) $(N1K)
