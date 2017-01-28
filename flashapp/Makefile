APP=flashapp

# our code
OBJS =
OBJS += ../rad1olib/setup.o
OBJS += ../r0ketlib/display.o
OBJS += ../r0ketlib/render.o
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
OBJS += msc/msc_desc.o
OBJS += msc/msc_main.o
OBJS += msc/msc_ram.o
OBJS += ../lpcapi/usbd_common.o # MUST be before hackrf usb.o, because of USB0_IRQ selection
OBJS += ../hackrf/firmware/common/usb.o
OBJS += ../lpcapi/cdc/cdc_desc.o
OBJS += ../lpcapi/cdc/cdc_main.o
OBJS += ../lpcapi/cdc/cdc_vcom.o

# CPLD
OBJS += ../hackrf/firmware/common/xapp058/micro.o
OBJS += ../hackrf/firmware/common/xapp058/ports.o
OBJS += ../hackrf/firmware/common/xapp058/lenval.o
OBJS += ../hackrf/firmware/common/cpld_jtag.o
OBJS += ../hackrf/firmware/common/gpio_lpc.o
OBJS += ../hackrf/firmware/common/hackrf_core.o

OBJS += main.o

CFLAGS=-Wno-unused-variable -DRAD1O -DLPC43XX_M4 -Werror
OBJS += ../hackrf/firmware/common/w25q80bv.o
OBJS += ../hackrf/firmware/common/max2871.o


LDSCRIPT=../ld/app.ld
RPATH=..
include ../Makefile.inc

CPLDFILE=xsvf.inc
main.o: $(CPLDFILE)

xsvf.inc:
	cat ../hackrf/firmware/cpld/sgpio_if/default.xsvf |xxd -i  > $(CPLDFILE)

clean::
	$(RM) $(CPLDFILE)
