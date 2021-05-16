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
OBJS += ../r0ketlib/decoder.o
OBJS += ../rad1olib/spi-flash.o
OBJS += ../rad1olib/assert.o
OBJS += ../rad1olib/systick.o
OBJS += ../fatfs/diskio.o
OBJS += ../fatfs/ff.o
OBJS += ../r0ketlib/select.o
OBJS += ../r0ketlib/idle.o
OBJS += ../r0ketlib/fs_util.o
OBJS += ../lpcapi/usbd_common.o
OBJS += ../lpcapi/msc/msc_main.o
OBJS += ../lpcapi/msc/msc_ram.o
OBJS += ../lpcapi/msc/msc_desc.o
OBJS += ../hackrf-old/firmware/common/usb.o

CFLAGS=-Wno-unused-variable -DRAD1O -DLPC43XX_M4
OBJS += ../hackrf-old/firmware/common/w25q80bv.o
OBJS += ../hackrf-old/firmware/common//max2871.o
#OBJS += ../hackrf-old/firmware/common//max2871_regs.o

RPATH=..
LDSCRIPT=boot.ld
include ../Makefile.inc

