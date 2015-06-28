APP=main

# our code
OBJS = main.o
OBJS += feldtest.o
OBJS += ../rad1olib/setup.o
OBJS += ../r0ketlib/display.o
OBJS += ../r0ketlib/render.o
OBJS += ../r0ketlib/fonts/smallfonts.o
OBJS += ../r0ketlib/print.o
OBJS += ../r0ketlib/itoa.o
OBJS += ../r0ketlib/keyin.o
OBJS += ../r0ketlib/menu.o
OBJS += ../rad1olib/max2871.o
OBJS += ../rad1olib/max2871_regs.o
OBJS += ../rad1olib/spi-flash.o
OBJS += msc_desc.o
OBJS += msc_main.o
OBJS += msc_ram.o
OBJS += ../hackrf/firmware/common/si5351c.o
OBJS += ../hackrf/firmware/common/usb.o

INCLUDE_PATHS+=-I../hackrf/firmware/common
CFLAGS=-Wno-unused-variable -DRAD1O -DLPC43XX_M4
OBJS += ../hackrf/firmware/common/w25q80bv.o

RPATH=..
include ../Makefile.inc
