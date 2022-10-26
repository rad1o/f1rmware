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
	$(MAKE) -C rflib
	$(MAKE) -C l0unge
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

hackrf-old/Readme.md:
	git submodule init
	git submodule update

hackrf/Readme.md:
	git submodule init
	git submodule update

hackrf/firmware/libopencm3/README:
	cd hackrf && git submodule init && git submodule update

hackrf/firmware/hackrf_usb/build/hackrf_usb_ram.bin: hackrf/firmware/libopencm3/README
	mkdir -p hackrf/firmware/hackrf_usb/build
	cmake -B hackrf/firmware/hackrf_usb/build/ -S hackrf/firmware/hackrf_usb/ -DBOARD=RAD1O
	$(MAKE) -C hackrf/firmware/hackrf_usb/build/

hackrf: hackrf-old/Readme.md hackrf/Readme.md hackrf/firmware/hackrf_usb/build/hackrf_usb_ram.bin

clean:
	$(MAKE) -C campapp clean
	$(MAKE) -C ccccmaze clean
	$(MAKE) -C testapp clean
	$(MAKE) -C musicapp clean
	$(MAKE) -C rfapp clean
	$(MAKE) -C rflib clean
	$(MAKE) -C l0unge clean
	$(MAKE) -C bootloader clean
	$(MAKE) -C flashapp clean
	$(MAKE) -C l0dables clean
	$(MAKE) -C smartflash clean
	$(MAKE) -C hackrf-old hack-clean
	rm -rf  hackrf/firmware/hackrf_usb/build/
	$(MAKE) -C libopencm3 clean
