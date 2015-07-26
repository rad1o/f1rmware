RPATH?=.

CROSS_COMPILE = arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
AR = $(CROSS_COMPILE)ar
AS = $(CROSS_COMPILE)as
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE = $(CROSS_COMPILE)size
DFUTOOL= $(RPATH)/tools/make-dfu.py
DFUUTIL= dfu-util

INCLUDE_PATHS += -I$(RPATH)/libopencm3/include
INCLUDE_PATHS += -I$(RPATH)/hackrf/firmware
INCLUDE_PATHS += -I$(RPATH)

LFLAGS += -Os -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16

CFLAGS += -std=c99 -c -g -Wall -Wno-unused-function
CFLAGS += -fno-common -ffunction-sections -fdata-sections -fno-builtin -ffreestanding
#CFLAGS += -fmessage-length=0 
CFLAGS += $(LFLAGS)

LDSCRIPT?=$(RPATH)/libopencm3/lib/libopencm3_lpc43xx.ld

LDFLAGS += -nostartfiles $(LFLAGS)
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -T$(RPATH)/ld/LPC4330_M4_memory.ld 
LDFLAGS += -T$(LDSCRIPT)
LDFLAGS += -Wl,-Map=$(APP).map

LIBS += -L$(RPATH)/libopencm3/lib -lopencm3_lpc43xx
LIBS += -lc

OCFLAGS = --strip-unneeded

DEPDIR = .deps
df = $(DEPDIR)/$(*F)

%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDE_PATHS) -o $@ $<
	$(CC) -MM $(CFLAGS) $(INCLUDE_PATHS) $< > $(df).d

%.bin: %.elf
	$(OBJCOPY) $(OCFLAGS) -O binary $< $@

%.dfu: %.bin
	$(DFUTOOL) $< $@

all: $(DEPDIR) $(APP).dfu $(APP).bin

$(DEPDIR):
	mkdir $(DEPDIR)

clean::
	-rm -f $(OBJS) $(APP).elf $(APP).bin $(APP).dfu $(APP).map
	-rm -rf $(DEPDIR)

program: flash

flash: $(APP).dfu
	$(DFUUTIL) --device 1fc9:000c --alt 0 --download $(APP).dfu

$(APP).elf: $(DEPDIR) $(OBJS) $(LDSCRIPT)
	$(CC) $(LDFLAGS) -o $(APP).elf $(OBJS) $(LIB_PATHS) $(LIBS)
	$(SIZE) $(APP).elf

-include $(SRCS:%.c=$(DEPDIR)/%.d)
