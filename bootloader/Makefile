APP=main

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
OBJS += ../rad1olib/max2871.o
OBJS += ../rad1olib/max2871_regs.o
OBJS += ../rad1olib/spi-flash.o
OBJS += ../rad1olib/assert.o
#OBJS += ../hackrf/firmware/common/si5351c.o
#OBJS += ../hackrf/firmware/common/usb.o
OBJS += ../fatfs/diskio.o
OBJS += ../fatfs/ff.o
OBJS += ../r0ketlib/select.o
OBJS += ../r0ketlib/idle.o

INCLUDE_PATHS+=-I../hackrf/firmware/common
CFLAGS=-Wno-unused-variable -DRAD1O -DLPC43XX_M4
OBJS += ../hackrf/firmware/common/w25q80bv.o

RPATH=..
include ../Makefile.inc

LDFLAGS =
LDFLAGS += -nostartfiles $(LFLAGS)
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -T$(RPATH)/ld/LPC4330_M4_memory.ld 
LDFLAGS += -Tboot.ld
LDFLAGS += -Wl,-Map=$(APP).map

