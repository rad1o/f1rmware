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
OBJS += ../hackrf/firmware/hackrf_usb/hackrf_usb.o
OBJS += ../hackrf/firmware/hackrf_usb/sgpio_isr.o
OBJS += ../hackrf/firmware/hackrf_usb/usb_api_board_info.o
OBJS += ../hackrf/firmware/hackrf_usb/usb_api_cpld.o
OBJS += ../hackrf/firmware/hackrf_usb/usb_api_register.o
OBJS += ../hackrf/firmware/hackrf_usb/usb_api_spiflash.o
OBJS += ../hackrf/firmware/hackrf_usb/usb_api_transceiver.o
OBJS += ../hackrf/firmware/hackrf_usb/usb_api_sweep.o
OBJS += ../hackrf/firmware/hackrf_usb/usb_api_operacake.o
OBJS += ../hackrf/firmware/hackrf_usb/usb_bulk_buffer.o
OBJS += ../hackrf/firmware/hackrf_usb/usb_descriptor.o
OBJS += ../hackrf/firmware/hackrf_usb/usb_device.o
OBJS += ../hackrf/firmware/hackrf_usb/usb_endpoint.o
OBJS += ../hackrf/firmware/common/operacake.o
OBJS += ../hackrf/firmware/common/usb_standard_request.o
OBJS += ../hackrf/firmware/common/usb_queue.o
OBJS += ../hackrf/firmware/common/usb_request.o
OBJS += ../hackrf/firmware/common/usb.o
OBJS += ../hackrf/firmware/common/rom_iap.o
OBJS += ../hackrf/firmware/common/xapp058/micro.o
OBJS += ../hackrf/firmware/common/xapp058/ports.o
OBJS += ../hackrf/firmware/common/xapp058/lenval.o
OBJS += ../hackrf/firmware/common/cpld_jtag.o
OBJS += ../hackrf/firmware/common/si5351c.o
OBJS += ../hackrf/firmware/common/gpdma.o
OBJS += ../hackrf/firmware/common/tuning.o
OBJS += ../hackrf/firmware/common/max2837.o
OBJS += ../hackrf/firmware/common/rf_path.o
OBJS += ../hackrf/firmware/common/max5864.o
OBJS += ../hackrf/firmware/common/hackrf_core.o
OBJS += ../hackrf/firmware/common/sgpio.o
OBJS += ../hackrf/firmware/common/streaming.o
OBJS += ../hackrf/firmware/common/w25q80bv.o
OBJS += ../hackrf/firmware/common/max2871.o
OBJS += ../hackrf/firmware/common/max2871_regs.o
OBJS += ../hackrf/firmware/common/mixer.o
OBJS += ../hackrf/firmware/common/gpio_lpc.o
OBJS += ../hackrf/firmware/common/spi_bus.o
OBJS += ../hackrf/firmware/common/spi_ssp.o
OBJS += ../hackrf/firmware/common/i2c_bus.o
OBJS += ../hackrf/firmware/common/i2c_lpc.o
OBJS += ../hackrf/firmware/common/max2837_target.o
OBJS += ../hackrf/firmware/common/max5864_target.o
OBJS += ../hackrf/firmware/common/w25q80bv_target.o

SRCS = $(wildcard *.c)
SOBJS = $(foreach mod,$(SRCS),$(subst .c,.o,$(mod)))

OBJS += $(SOBJS)

REVISION = $(shell git rev-parse --short HEAD)

CFLAGS=-Wno-unused-variable -DRAD1O -DLPC43XX_M4 -DVERSION_STRING=\"git-$(REVISION)\"

CFLAGS+=-I../hackrf/firmware/common

LIBS += -lm

LDSCRIPT=../ld/app.ld
RPATH=..
include ../Makefile.inc
