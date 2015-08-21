all: lib hackrf subdirs

travis:
	$(MAKE) ADDFLAGS=-Werror lib hackrf subdirs
	$(MAKE) -C smartflash test

subdirs:
	$(MAKE) -C campapp
	$(MAKE) -C ccccmaze
	$(MAKE) -C testapp
	$(MAKE) -C musicapp
	$(MAKE) -C rfapp
	$(MAKE) -C bootloader
	$(MAKE) -C flashapp
	$(MAKE) -C l0dables
	$(MAKE) -C smartflash

libopencm3/README:
	git submodule init
	git submodule update

libopencm3/lib/libopencm3_lpc43xx.a:
	$(MAKE) -C libopencm3

lib: libopencm3/README libopencm3/lib/libopencm3_lpc43xx.a

hackrf/Readme.md:
	git submodule init
	git submodule update

hackrf/firmware/hackrf_usb/build/hackrf_usb.bin:
	$(MAKE) -C hackrf

hackrf: hackrf/Readme.md hackrf/firmware/hackrf_usb/build/hackrf_usb.bin

clean:
	$(MAKE) -C campapp clean
	$(MAKE) -C ccccmaze clean
	$(MAKE) -C testapp clean
	$(MAKE) -C musicapp clean
	$(MAKE) -C rfapp clean
	$(MAKE) -C bootloader clean
	$(MAKE) -C flashapp clean
	$(MAKE) -C l0dables clean
	$(MAKE) -C smartflash clean
	$(MAKE) -C hackrf hack-clean
#	cd libopencm3 && make clean
