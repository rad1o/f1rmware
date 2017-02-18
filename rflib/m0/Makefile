# derived primarily from Makefiles in libopencm3
BINARY = m0
SRC = m0.c cossin1024.c \
	hackrf_core.c max2837.c max2871.c max2871_regs.c max5864.c rf_path.c sgpio.c si5351c.c tuning.c gpio_lpc.c mixer.c \
	max2837_target.c max5864_target.c i2c_bus.c i2c_lpc.c spi_bus.c spi_ssp.c

LDSCRIPT ?= ../../ld/m0.ld

LIBOPENCM3 ?= ../../libopencm3

PREFIX		?= arm-none-eabi
CC		= $(PREFIX)-gcc
LD		= $(PREFIX)-gcc
OBJCOPY		= $(PREFIX)-objcopy
OBJDUMP		= $(PREFIX)-objdump
GDB		= $(PREFIX)-gdb
TOOLCHAIN_DIR := $(shell dirname `which $(CC)`)/../$(PREFIX)

CFLAGS += -std=gnu99 -Os -g2 -Wall -Wextra -I$(LIBOPENCM3)/include \
	-mthumb -DTHUMB -mcpu=cortex-m0 -flto -ffunction-sections -fdata-sections \
	-DLPC43XX -DLPC43XX_M0 -DCORE_M0 -D__CORTEX_M=0 -DRAD1O \
	-I. -I../.. -I../../portalib -I../../rfapp -I../../hackrf/firmware/common

LDFLAGS += -mcpu=cortex-m0 -mthumb -DTHUMB \
		-L../../ld \
		-L$(LIBOPENCM3)/lib -L$(LIBOPENCM3)/lib/lpc43xx_m0 \
		-T$(LDSCRIPT) -nostartfiles -lc -lnosys \
		-Wl,--gc-sections -Xlinker -Map=$(BINARY).map

OBJ += $(SRC:.c=.o)

# Be silent per default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
Q := @
NULL := 2>/dev/null
else
LDFLAGS += -Wl,--print-gc-sections
endif

.SUFFIXES: .elf .bin .list
.SECONDEXPANSION:
.SECONDARY:

all: $(BINARY).bin

%.bin: %.elf
	@#printf "  OBJCOPY $(*).bin\n"
	$(Q)$(OBJCOPY) -Obinary $(*).elf $(*).bin

%.list: %.elf
	@#printf "  OBJDUMP $(*).list\n"
	$(Q)$(OBJDUMP) -S $(*).elf > $(*).list

%.elf: $(OBJ) $(LDSCRIPT)
	@#printf "  LD      $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(LD) $(LDFLAGS) -o $(*).elf $(OBJ) -lopencm3_lpc43xx_m0

%.o: %.s Makefile
	@#printf "  CC      $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(CC) $(CFLAGS) -o $@ -c $<

clean:
	$(Q)rm -f *.o
	$(Q)rm -f *.d
	$(Q)rm -f *.elf
	$(Q)rm -f *.bin
	$(Q)rm -f *.dfu
	$(Q)rm -f _tmp.dfu _header.bin
	$(Q)rm -f *.hex
	$(Q)rm -f *.srec
	$(Q)rm -f *.list
	$(Q)rm -f *.map
	$(Q)rm -f *.lst
	$(Q)rm -f ../common/*.o
	$(Q)rm -f ../common/*.d
	$(Q)rm -f ../common/*.lst

MAKE_ALL_RULE_HOOK:

FORCE:
.PHONY: FORCE all clean

-include $(OBJ:.o=.d)
