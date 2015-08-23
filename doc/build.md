# Build instructions

## Required software

* arm-gcc: (https://launchpad.net/gcc-arm-embedded)
    * Manual installation:

      ```
      cd projectdir
      wget https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q2-update/+download/gcc-arm-none-eabi-4_9-2015q2-20150609-linux.tar.bz2
      tar -xjf gcc-arm-none-eabi-4_9-2015q2-20150609-linux.tar.bz2
      export PATH=`pwd`/gcc-arm-none-eabi-4_9-2015q2/bin:$PATH
      ```
    * Debian/Ubuntu: 
        * Ubuntu 14.04 users start with:

          ```
          sudo add-apt-repository -y ppa:terry.guo/gcc-arm-embedded
          echo -e "Package: gcc-arm-none-eabi\n Pin: release o=LP-PPA-terry.guo-gcc-arm-embedded\n Priority: 501" |sudo tee /etc/apt/preferences.d/pin-gcc-arm-embedded
          sudo apt-get update
          ```
      `sudo apt-get install gcc-arm-none-eabi libnewlib-arm-none-eabi`
    * Arch: `sudo pacman -S arm-none-eabi-gcc arm-none-eabi-newlib arm-none-eabi-binutils`
    * FreeBSD: Theory: `sudo pkg install arm-none-eabi-gcc492 arm-none-eabi-binutils` / `portmaster devel/arm-none-eabi-gcc492 devel/arm-none-eabi-binutils`; practice: `kldload linux` & see _Manual installation_
    * Fedora: `sudo dnf install arm-none-eabi-newlib arm-none-eabi-gcc-cs-c++.x86_64 gcc-c++-arm-linux-gnu.x86_64`
    ** note: [https://bugzilla.redhat.com/show_bug.cgi?id=1248294](RHBZ #1248294] you will need to rebuild arm-none-eabi-newlib as described by the reporter. Or just use [https://blog.pcfe.net/packages/arm-none-eabi-newlib-2.2.0_1-4.fc24.noarch.rpm](my build).
* python-yaml (http://pyyaml.org/)
    * Debian/Ubuntu: `sudo apt-get install python-yaml`
    * Arch: `sudo pacman -S python-yaml`
    * FreeBSD: `sudo pkg install py27-yaml` or `portmaster devel/py-yaml`
    * Fedora: `sudo dnf install PyYAML`
    * Alternative: via pip/virtualenv
* to build docs
    * Fedora: `sudo dnf install doxygen graphviz`
* libopencm3 (fork, https://github.com/rad1o/libopencm3)
    * If you are using `git`, the preferred way to install
      `libopencm3` is to use the submodule:

      ```
      git clone https://github.com/rad1o/f1rmware.git
      cd f1rmware
      git submodule init
      git submodule update
      cd libopencm3
      make
      ```
* xxd (optional, needed for flashapp / flash-station setup)
    * Debian: probably already installed (is a part of vim-common package)
    * Arch: `yaourt -S xxd`
    * FreeBSD: part of `vim-lite` package / `editors/vim-lite` port
* dfu-util (optional, convenient for development, http://dfu-util.sourceforge.net/)
    * Debian/Ubuntu: `sudo apt-get install dfu-util`
    * Arch: `sudo pacman -S dfu-util`
    * FreeBSD: `pkg install dfu-util` / `portmaster ports/dfu-util`
    * Fedora: `sudo dnf install dfu-util`

    * If you want to use smartflash, instead build and install our fork from https://github.com/rad1o/dfu-util

      ```
      git clone https://github.com/rad1o/dfu-util.git
      sudo apt-get install autoconf libusb-1.0-0-dev
      cd dfu-util && sh autogen.sh && ./configure && make
      sudo make install
      ```

* cmake (optional, if you want to build the hackrf firmware)
    * `sudo apt-get install cmake`
    * Fedora: `sudo dnf install cmake`

If you use [Nix or NixOS](https://nixos.org/), you can do `nix-shell --pure .` to enter a shell with all required dependencies.

## Build firmware

```
git clone https://github.com/rad1o/f1rmware.git
cd f1rmware
make
```

On BSD systems, use `gmake` instead of `make`.

## continue with instructions on wiki
Now that your toolchain is set up under Linux, follow the [instructions on the wiki](https://rad1o.badge.events.ccc.de/howto:build) to build regularly from git and flash your rad1o.

## Flash rad1o

Prerequisite: rad1o is connected to USB

To flash the firmware permanently, simply copy the `smartflash/IMG/*.b1n` files to the mounted mass storage of rad1o.
If you want to just run the firmware once without permanently storing it on the rad1o, you will need the `dfu-util` mentioned above and run the `make flash` command.

Alternatively, you can use `FlashGUI` in the `smartflash` directory, see `smartflash/README.md` for details.

To fix issues on some boards that are not flashable by FlashGUI or to restore the filesystem on the radio in general, run `sudo mkfs.msdos -I /dev/sdx` and restore the firmware again with `FlashGUI`. 


## OS X setup:

Install homebrew if you haven't already.

```
ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
brew tap PX4/homebrew-px4
brew update
brew install gcc-arm-none-eabi-49
brew install dfu-util
brew install python
which python #should be /usr/local/bin/python, otherwise add export PATH=/usr/local/bin/:$PATH to your shell startup file, e.g, .zlogin or .bash_profile
pip install pyaml
```

Goto your projects folder, for example:

```
mkdir $HOME/rad1o
cd $HOME/rad1o
```

Get the firmware, compile and flash it.

```
git clone https://github.com/rad1o/f1rmware.git
cd f1rmware/
make
make flash
```

If you don't have the ARM toolchain, an easy way to get it on OS X is to download the [Energia IDE](http://energia.nu/download) and add `/Applications/Energia.app/Contents/Resources/Java/hardware/tools/lm4f/bin/` to your `PATH`.
