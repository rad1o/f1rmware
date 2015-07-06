APP=boot

# our code
OBJS = main.o
OBJS += ../rad1olib/setup.o
OBJS += ../r0ketlib/display.o
OBJS += ../r0ketlib/render.o
OBJS += ../r0ketlib/fonts/smallfonts.o
OBJS += ../r0ketlib/print.o
OBJS += ../r0ketlib/itoa.o
OBJS += ../r0ketlib/keyin.o
OBJS += ../r0ketlib/menu.o
OBJS += ../rad1olib/spi-flash.o
OBJS += ../rad1olib/assert.o
OBJS += ../fatfs/diskio.o
OBJS += ../fatfs/ff.o
OBJS += ../r0ketlib/select.o
OBJS += ../r0ketlib/idle.o
OBJS += ../r0ketlib/fs_util.o

CFLAGS=-Wno-unused-variable -DRAD1O -DLPC43XX_M4
OBJS += ../hackrf/firmware/common/w25q80bv.o
OBJS += ../hackrf/firmware/common//max2871.o
#OBJS += ../hackrf/firmware/common//max2871_regs.o

RPATH=..
LDSCRIPT=boot.ld
include ../Makefile.inc

