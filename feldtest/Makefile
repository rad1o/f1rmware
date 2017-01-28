APP=feldtest

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
OBJS += ../rad1olib/spi-flash.o
OBJS += ../rad1olib/assert.o
OBJS += ../rad1olib/systick.o
OBJS += msc_desc.o
OBJS += msc_main.o
OBJS += msc_ram.o
OBJS += ../hackrf/firmware/common/si5351c.o
OBJS += ../hackrf/firmware/common/i2c_bus.o
OBJS += ../hackrf/firmware/common/i2c_lpc.o
OBJS += ../hackrf/firmware/common/usb.o
OBJS += ../fatfs/diskio.o
OBJS += ../fatfs/ff.o
OBJS += ../r0ketlib/select.o
OBJS += ../r0ketlib/idle.o
OBJS += ../r0ketlib/fs_util.o
OBJS += ../r0ketlib/decoder.o

CFLAGS=-Wno-unused-variable -DRAD1O -DLPC43XX_M4
OBJS += ../hackrf/firmware/common/w25q80bv.o
OBJS += ../hackrf/firmware/common/max2871.o
#OBJS += ../hackrf/firmware/common/max2871_regs.o

LDSCRIPT=../ld/app.ld
RPATH=..
include ../Makefile.inc
