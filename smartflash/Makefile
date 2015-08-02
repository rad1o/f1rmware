# Makefile to create filesystem image and flash rad10 badge

default: gather image
all: build gather image run

fsdir?=IMG
mountpoint?=/mnt
fsimg?=filesystem.img
hdrimg?=header.img
fullimg?=full.img
APP=../flashapp/flashapp.dfu
FILEORDER= \
    hackrf.bin \
    testapp.bin 

gather:
	@echo
	@echo '### Gathering contents for image'
	[ ! -d $(fsdir) ] && mkdir $(fsdir) || true
	cp ../hackrf/firmware/hackrf_usb/build/hackrf_usb.bin $(fsdir)/hackrf.bin
	cp ../testapp/testapp.bin $(fsdir)/
	-cp ../l0dables/*.c1d $(fsdir)/
	cp ../hackrf/firmware/cpld/sgpio_if/default.xsvf $(fsdir)/cpld.xsv
	# Add f0nts, images, l0dables, etc.
	echo -n "testapp.bin" > $(fsdir)/boot.cfg
	cp ../flashapp/flashapp.dfu .

fsimage: FILES!= perl order-files $(fsdir) $(FILEORDER)
fsimage:
	@echo
	@echo '### Creating FS image'
	$(RM) -f $(fsimg)
	mkfs.fat -F 12 -I -S 512 -C $(fsimg) 1536
	mcopy -i $(fsimg) $(FILES) ::

header:
	@echo
	@echo '### Creating boot header image'
	dd if=/dev/zero bs=1024 count=512 |tr "\000" "\377" > $(hdrimg)
	dd if=../bootloader/boot.bin of=$(hdrimg) conv=notrunc

image: fsimage header
	@echo
	@echo '### Assembling full image'
	dd if=$(hdrimg) bs=1024 of=$(fullimg)
	dd if=$(fsimg) bs=1024 seek=512 of=$(fullimg)

run: image
	./FLASHgui

### Rebuild everything 
rebuild: build gather image
build:
	@echo
	@echo '### Building everything'
	cd ../hackrf/firmware/hackrf_usb/build && make
	cd ../testapp && make testapp.bin
	cd ../bootloader && make boot.bin
	cd ../flashapp && make flashapp.dfu
	cd ../l0dables && make

dfu-util:
	cd ../../dfu-util && sh autogen.sh && ./configure && make

clean:
	$(RM) -rf $(fsdir)
	$(RM) $(fsimg) $(hdrimg) $(fullimg) flashapp.dfu


### Targets for FLASHgui
flash:
	dd if=$(fullimg) bs=512 of=/dev/$(UDISK) conv=fsync 2>/dev/null

dfu:
	dfu-util -p $(UPATH) --download $(APP) 2>&1 >/dev/null  | grep -v "unable to read DFU status after completion" || true

msc: FILES!= perl order-files $(fsdir) $(FILEORDER)

msc:
	mcopy -D o -i /dev/$(UDISK) $(FILES) ::
