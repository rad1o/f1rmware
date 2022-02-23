APP=hkrf-app

OBJS =

# our code
OBJS += ../rad1olib/sbrk.o
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
OBJS += ../fatfs/diskio.o
OBJS += ../fatfs/ff.o
OBJS += ../r0ketlib/select.o
OBJS += ../r0ketlib/idle.o
OBJS += ../r0ketlib/fs_util.o
OBJS += ../r0ketlib/execute.o
OBJS += ../r0ketlib/config.o
OBJS += ../r0ketlib/stringin.o
OBJS += ../r0ketlib/colorin.o
OBJS += ../r0ketlib/intin.o
OBJS += ../r0ketlib/random.o
OBJS += ../r0ketlib/image.o
OBJS += ../r0ketlib/mesh.o
OBJS += ../r0ketlib/night.o
OBJS += ../rad1olib/light_ws2812_cortex.o
OBJS += ../rad1olib/battery.o
OBJS += ../rad1olib/draw.o

# Hackrf code
OBJS += ../hackrf-old/firmware/hackrf_usb/hackrf_usb.o
OBJS += ../hackrf-old/firmware/hackrf_usb/sgpio_isr.o
OBJS += ../hackrf-old/firmware/hackrf_usb/usb_api_board_info.o
OBJS += ../hackrf-old/firmware/hackrf_usb/usb_api_cpld.o
OBJS += ../hackrf-old/firmware/hackrf_usb/usb_api_register.o
OBJS += ../hackrf-old/firmware/hackrf_usb/usb_api_spiflash.o
OBJS += ../hackrf-old/firmware/hackrf_usb/usb_api_transceiver.o
OBJS += ../hackrf-old/firmware/hackrf_usb/usb_bulk_buffer.o
OBJS += ../hackrf-old/firmware/hackrf_usb/usb_descriptor.o
OBJS += ../hackrf-old/firmware/hackrf_usb/usb_device.o
OBJS += ../hackrf-old/firmware/hackrf_usb/usb_endpoint.o
OBJS += ../hackrf-old/firmware/common/usb_standard_request.o
OBJS += ../hackrf-old/firmware/common/usb_queue.o
OBJS += ../hackrf-old/firmware/common/usb_request.o
OBJS += ../hackrf-old/firmware/common/usb.o
OBJS += ../hackrf-old/firmware/common/rom_iap.o
OBJS += ../hackrf-old/firmware/common/xapp058/micro.o
OBJS += ../hackrf-old/firmware/common/xapp058/ports.o
OBJS += ../hackrf-old/firmware/common/xapp058/lenval.o
OBJS += ../hackrf-old/firmware/common/cpld_jtag.o
OBJS += ../hackrf-old/firmware/common/si5351c.o
OBJS += ../hackrf-old/firmware/common/sgpio_dma.o
OBJS += ../hackrf-old/firmware/common/gpdma.o
OBJS += ../hackrf-old/firmware/common/tuning.o
OBJS += ../hackrf-old/firmware/common/max2837.o
OBJS += ../hackrf-old/firmware/common/rf_path.o
OBJS += ../hackrf-old/firmware/common/max5864.o
OBJS += ../hackrf-old/firmware/common/hackrf_core.o
OBJS += ../hackrf-old/firmware/common/sgpio.o
OBJS += ../hackrf-old/firmware/common/streaming.o
OBJS += ../hackrf-old/firmware/common/w25q80bv.o
OBJS += ../hackrf-old/firmware/common/max2871.o

SRCS = $(wildcard *.c)
SOBJS = $(foreach mod,$(SRCS),$(subst .c,.o,$(mod)))

OBJS += $(SOBJS)

REVISION = $(shell git rev-parse --short HEAD)

CFLAGS=-Wno-unused-variable -DRAD1O -DLPC43XX_M4 -DVERSION_STRING=\"git-$(REVISION)\"

CFLAGS+=-I../hackrf-old/firmware/common

LIBS += -lm

LDFLAGS += --specs=nosys.specs

LDSCRIPT=../ld/app.ld
RPATH=..
include ../Makefile.inc
