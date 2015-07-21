# Build instructions

## Required software
* arm-gcc: (https://launchpad.net/gcc-arm-embedded)
    * Manual installation:
        * `wget https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q2-update/+download/gcc-arm-none-eabi-4_9-2015q2-20150609-linux.tar.bz2`
        * `export PATH=$HOME/rad1o/gcc-arm-none-eabi-4_9-2015q2/bin:$PATH`
    * Debian/Ubuntu: `sudo apt-get install gcc-arm-none-eabi libnewlib-arm-none-eabi`
    * Arch: `sudo pacman -S arm-none-eabi-gcc arm-none-eabi-newlib arm-none-eabi-binutils`
* python-yaml (http://pyyaml.org/)
    * Debian/Ubuntu: `sudo apt-get install python-yaml`
    * Arch: `sudo pacman -S python-yaml`
    * Alternative: via pip/virtualenv
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
* dfu-util (optional, convenient for development, http://dfu-util.sourceforge.net/)
    * Debian/Ubuntu: `sudo apt-get install dfu-util`
    * Arch: `sudo pacman -S dfu-util`


## Build firmware

```
git clone https://github.com/rad1o/f1rmware.git
cd f1rmware
make
```


## Flash rad1o

Prerequisite: rad1o is connected to USB

```
sudo make flash
```


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
