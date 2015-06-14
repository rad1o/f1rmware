CC = arm-none-eabi-gcc
LD = arm-none-eabi-gcc
AR = arm-none-eabi-ar
AS = arm-none-eabi-as
OBJCOPY = arm-none-eabi-objcopy
OD = arm-none-eabi-objdump
SIZE = arm-none-eabi-size

OCFLAGS = --strip-unneeded

CFLAGS = -I../libopencm3/include/
CFLAGS += -I../common/
CFLAGS += -I../lcd/
CFLAGS += -I../blinky/
CFLAGS += -I./ -c -fno-common -Os -g -mcpu=cortex-m4 -mthumb -Wall -ffunction-sections -fdata-sections -fno-builtin -Wno-unused-function -ffreestanding
CFLAGS += -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -std=c99
LFLAGS = -TLPC4330_M4_memory.ld -nostartfiles -Wl,--gc-sections -mthumb

LFLAGS = -nostartfiles -Wl,--gc-sections -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
LFLAGS+= -T../ld/LPC4320_M4_memory.ld -T../libopencm3/lib/libopencm3_lpc43xx.ld -T../ld/LPC43xx_M4_M0_image_from_text.ld
LIBS= -L../libopencm3/lib -Xlinker -Map=main.map -L/usr/arm-none-eabi/lib/armv7e-m/fpu  -lc -lnosys -lopencm3_lpc43xx -lm 



# our code
OBJS = main.o

OBJS += ../common/setup.o
OBJS += ../lcd/display.o
OBJS += ../lcd/render.o
OBJS += ../lcd/fonts/smallfonts.o
OBJS += ../lcd/print.o
OBJS += ../lcd/itoa.o
OBJS += ../blinky/keyin.o
OBJS += feldtest.o
OBJS += max2871.o
OBJS += si5351c.o


all: main.dfu

clean:
	-rm -f $(OBJS) main.lst main.elf main.hex main.map main.bin main.list main.dfu _tmp.dfu _header.bin

main.elf: $(OBJS)
	$(LD) $(LFLAGS) -o main.elf $(OBJS) $(LIBS)
	-@echo ""
	$(SIZE) main.elf
	-@echo ""

%.o : %.c
	$(CC) $(CFLAGS) -o $@ $<

%.bin: %.elf
	$(OBJCOPY) $(OCFLAGS) -O binary $< $@

%.dfu: %.bin
	cp $< _tmp.dfu
	dfu-suffix --vid=0x1fc9 --pid=0x000c --did=0x0 -a _tmp.dfu
	python2 -c "import os.path; import struct; print('0000000: da ff ' + ' '.join(map(lambda s: '%02x' % ord(s), struct.pack('<H', os.path.getsize('$<') / 512 + 1))) + 'ff ff ff ff ff ff ff ff ff ff ff ff')" | xxd -g1 -r > _header.bin
	cat _header.bin _tmp.dfu > $@

program: main.dfu
	sudo dfu-util --device 1fc9:000c --alt 0 --download main.dfu
