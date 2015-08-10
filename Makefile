all: lib hackrf subdirs

subdirs:
	$(MAKE) -C testapp
	$(MAKE) -C bootloader
	$(MAKE) -C flashapp
	$(MAKE) -C l0dables
	$(MAKE) -C smartflash

libopencm3/README:
	git submodule init
	git submodule update

libopencm3/lib/libopencm3_lpc43xx.a:
	cd libopencm3 && make

lib: libopencm3/README libopencm3/lib/libopencm3_lpc43xx.a

hackrf/Readme.md:
	git submodule init
	git submodule update

hackrf/hackrf.b1n:
	$(MAKE) -C hackrf

hackrf: hackrf/Readme.md hackrf/hackrf.b1n

clean:
	$(MAKE) -C testapp clean
	$(MAKE) -C bootloader clean
	$(MAKE) -C flashapp clean
	$(MAKE) -C l0dables clean
	$(MAKE) -C smartflash clean
	$(MAKE) -C hackrf hack-clean
#	cd libopencm3 && make clean
