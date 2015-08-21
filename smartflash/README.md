# smartflash

A fully automatic rad1o mass flash tool

## Description
smartflash detects rad1os which are attached via USB and which are either in DFU mode or USB Mass Storage (MSC) mode.

It then proceeds to automatically flash all files which are needed to use the rad1o.

## Modes
There are two different modes of operation. They are auto selected based on the mode of the rad1o (DFU or MSC)

### DFU Mode:
  - Programs the on-board CPLD
  - Formats the external 2 MB flash
  - Copies all gathered files to the external flash
  - Selects the camp application as default boot option


### MSC Mode:
  - Copies all gathered files to the external flash

## Prerequisites

A few tools and files need to be installed. The f1rmware and all other files need to be built too.

### Tools

#### mtools
 - On arch: `pacman -S mtools`

#### Patched dfu-util
The patched dfu-util provides support to flash more than one rad1o at a time.
It can be build from rad1o/dfu-util:
``` sh
git clone https://github.com/rad1o/dfu-util.git
cd dfu-util
sh autogen.sh
./configure  
make
# install in-place of existing dfu-util
sudo cp src/dfu-util `which dfu-util`
# or install
sudo make install
```
#### udev rule:
 - Copy `90-rad1o-flash.rules` to `/etc/udev/rules.d`


#### perl Curses module

#####Debian, Ubuntu, etc.
```
apt-get install libcurses-perl
```

#####arch

```
pacman -S perl-curses
```

#####Directly from CPAN

Note: You should probably only do this if there is no package for your distro.

```
perl -MCPAN -eshell # use default configuration, it's sufficient

# a new console, e.g. ' cpan[1]> ' opens, just type:

install Curses
exit
```

### Building the firmware and gathering all files:
 - Make sure to run "make" in the top level directory first. It will download and build all dependencies of the firmware
 - Change to this directory
 - Run "make build" to make sure hat all requisites are built
 - Run "make" to prepare everything for flashing

## Starting the process:
 - Run "make run" to start the CLI tool which will flash the rad1os
