APP=testapp

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
OBJS += ../r0ketlib/execute.o
OBJS += ../r0ketlib/config.o
OBJS += ../r0ketlib/stringin.o
OBJS += ../r0ketlib/colorin.o
OBJS += ../r0ketlib/random.o
OBJS += ../r0ketlib/image.o
OBJS += ../r0ketlib/mesh.o
OBJS += ../r0ketlib/night.o
OBJS += ../lpcapi/msc/msc_desc.o
OBJS += ../lpcapi/msc/msc_main.o
OBJS += ../lpcapi/msc/msc_ram.o
OBJS += ../lpcapi/usbd_common.o # MUST be before hackrf usb.o, because of USB0_IRQ selection
OBJS += ../hackrf/firmware/common/usb.o
OBJS += ../lpcapi/cdc/cdc_desc.o
OBJS += ../lpcapi/cdc/cdc_main.o
OBJS += ../lpcapi/cdc/cdc_vcom.o
OBJS += ../rad1olib/light_ws2812_cortex.o
OBJS += ../rad1olib/battery.o
OBJS += ../rad1olib/draw.o

# CPLD
OBJS += ../hackrf/firmware/common/xapp058/micro.o
OBJS += ../hackrf/firmware/common/xapp058/ports.o
OBJS += ../hackrf/firmware/common/xapp058/lenval.o
OBJS += ../hackrf/firmware/common/cpld_jtag.o

SRCS = $(wildcard *.c)
SOBJS = $(foreach mod,$(SRCS),$(subst .c,.o,$(mod)))

OBJS += $(SOBJS)

CFLAGS=-Wno-unused-variable -DRAD1O -DLPC43XX_M4
OBJS += ../hackrf/firmware/common/w25q80bv.o
OBJS += ../hackrf/firmware/common/max2871.o

OBJS += ../l0dables/jumptable.o

CFLAGS+=-I../portalib
CFLAGS+=-I../hackrf/firmware/common
OBJS += ../portalib/portapack.o
OBJS += ../portalib/rx_am.o
OBJS += ../portalib/rx_fm_broadcast.o
OBJS += ../portalib/rx_fm_narrowband.o
OBJS += ../portalib/decimate.o
OBJS += ../portalib/demodulate.o
OBJS += ../portalib/fxpt_atan2.o
OBJS += ../portalib/filters.o
OBJS += ../portalib/portapack_state.o
OBJS += ../portalib/specan.o
OBJS += ../portalib/fft.o
OBJS += ../hackrf/firmware/common/sgpio_dma.o
OBJS += ../hackrf/firmware/common/gpdma.o
OBJS += ../hackrf/firmware/common/tuning.o
OBJS += ../hackrf/firmware/common/max2837.o
OBJS += ../hackrf/firmware/common/rf_path.o
OBJS += ../hackrf/firmware/common/max5864.o
OBJS += ../hackrf/firmware/common/hackrf_core.o
OBJS += ../hackrf/firmware/common/sgpio.o
LIBS += -lm

LDSCRIPT=../ld/app.ld
RPATH=..
include ../Makefile.inc

main.o: main.gen

main.gen: $(SRCS)
	../tools/mkfirmware.pl $(SRCS) > main.gen

clean::
	$(RM) main.gen
