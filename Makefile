APP?=feldtest

all: lib hackrf $(APP).dfu $(APP).bin

libopencm3/README:
	git submodule init
	git submodule update

libopencm3/lib/libopencm3_lpc43xx.a:
	cd libopencm3 && make

lib: libopencm3/README libopencm3/lib/libopencm3_lpc43xx.a

hackrf/Readme.md:
	git submodule init
	git submodule update

hackrf: hackrf/Readme.md

$(APP)/$(APP).bin:
	$(MAKE) -C $(APP) $(APP).bin
	
$(APP)/$(APP).dfu:
	$(MAKE) -C $(APP) $(APP).dfu
	
$(APP).bin: $(APP)/feldtest.bin
	cp $< $@

$(APP).dfu: $(APP)/feldtest.dfu
	cp $< $@

clean:
	rm -f *.bin *.dfu
	$(MAKE) -C $(APP) clean
#	cd libopencm3 && make clean

flash: $(APP).dfu
    $(DFUUTIL) --device 1fc9:000c --alt 0 --download $(APP).dfu
