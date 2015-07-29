# Makefile to create filesystem image to flash rad1o badge

all: gather image

fsdir=IMG
mountpoint=/mnt
fsimg=filesystem.img
hdrimg=header.img
fullimg=full.img

gather:
	@echo
	@echo '### Gathering contents for image'
	[ ! -d $(fsdir) ] && mkdir $(fsdir) || true
	cp ../hackrf/firmware/hackrf_usb/build/hackrf_usb.bin $(fsdir)/hackrf.bin
	cp ../testapp/testapp.bin $(fsdir)/
	# Add f0nts, images, l0dables, etc.
	echo -n "testapp.bin" > $(fsdir)/boot.cfg
	cp ../flashapp/flashapp.dfu .

fsimage:
	@echo
	@echo '### Creating FS image'
	$(RM) -f $(fsimg)
	mkfs.fat -F 12 -I -S 512 -C $(fsimg) 1536
	sudo mount $(fsimg) $(mountpoint)
	sudo cp $(fsdir)/* $(mountpoint)
	sudo umount $(mountpoint)

header:
	@echo
	@echo '### Creating boot header image'
	dd if=/dev/zero bs=1024 count=512 |tr "\000" "\377" > $(hdrimg)
	dd if=../bootloader/boot.bin of=$(hdrimg) conv=notrunc

image: fsimage header
	@echo
	@echo '### Assembling Make full image'
	dd if=$(hdrimg) bs=1024 of=$(fullimg)
	dd if=$(fsimg) bs=1024 seek=512 of=$(fullimg)

### Rebuild everything 
build:
	cd ../hackrf/firmware/hackrf_usb/build && make
	cd ../testapp && make testapp.bin
	cd ../bootloader && make boot.bin
	cd ../flashapp && make flashapp.dfu

dfu-util:
	cd ../../dfu-util && sh autogen.sh && ./configure && make

clean:
	$(RM) -rf $(fsdir)
	$(RM) $(fsimg) $(hdrimg) $(fullimg) flashapp.dfu
