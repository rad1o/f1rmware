APP=ccccmaze

# our code
OBJS =
OBJS += ../rad1olib/setup.o
OBJS += ../r0ketlib/display.o
OBJS += ../r0ketlib/render.o
OBJS += ../rad1olib/sbrk.o
OBJS += ../r0ketlib/fonts/smallfonts.o
OBJS += ../r0ketlib/fonts/orbitron14.o
OBJS += ../r0ketlib/fonts/ubuntu18.o
OBJS += ../r0ketlib/decoder.o
OBJS += ../r0ketlib/print.o
OBJS += ../r0ketlib/itoa.o
OBJS += ../r0ketlib/keyin.o
OBJS += ../r0ketlib/menu.o
OBJS += ../rad1olib/spi-flash.o
OBJS += ../rad1olib/assert.o
OBJS += ../rad1olib/systick.o
OBJS += ../hackrf/firmware/common/si5351c.o
OBJS += ../fatfs/diskio.o
OBJS += ../fatfs/ff.o
OBJS += ../r0ketlib/select.o
OBJS += ../r0ketlib/idle.o
OBJS += ../r0ketlib/fs_util.o
OBJS += ../r0ketlib/execute.o
OBJS += ../r0ketlib/config.o
OBJS += ../r0ketlib/stringin.o
OBJS += ../r0ketlib/colorin.o
OBJS += ../r0ketlib/random.o
OBJS += ../r0ketlib/image.o
OBJS += ../r0ketlib/night.o
OBJS += ../rad1olib/light_ws2812_cortex.o
OBJS += ../rad1olib/battery.o
OBJS += ../rad1olib/draw.o

OBJS += ../hackrf/firmware/common/hackrf_core.o

OBJS += modules/navigate/display.o
OBJS += modules/navigate/initialize.o
OBJS += modules/navigate/maze.o
OBJS += modules/navigate/room.o
SRCS = $(wildcard *.c)
SOBJS = $(foreach mod,$(SRCS),$(subst .c,.o,$(mod)))

OBJS += $(SOBJS)

CFLAGS=-Wno-unused-variable -DRAD1O -DLPC43XX_M4
CFLAGS+=-I../hackrf/firmware/common

LIBS += -lm

LDSCRIPT=../ld/app.ld
RPATH=..
include ../Makefile.inc

main.o: main.gen

main.gen: $(SRCS)
	../tools/mkfirmware.pl $(SRCS) > main.gen

clean::
	$(RM) main.gen

install::
	mount /dev/$(shell dmesg | tail -n 10| grep '/1.50 MiB' | sed 's|.*\[||;s|].*||') /mnt
	cp ccccmaze.b1n /mnt
	rsync story/*.csm /mnt
	umount /mnt
