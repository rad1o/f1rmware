# rad1o

This is the f1rmware of the CCCamp 2015 radio badge.

## Project Infrastructure
Website: http://rad1o.badge.events.ccc.de/software

IRC channel: irc://irc.darkfasel.net/#rad1o (Port 6697 oder 9999, TLS-only, IPv6 Support)

## required software
* arm-gcc: https://launchpad.net/gcc-arm-embedded
    * manual installation
        * wget https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q2-update/+download/gcc-arm-none-eabi-4_9-2015q2-20150609-linux.tar.bz2
        * export PATH=$HOME/rad1o/gcc-arm-none-eabi-4_9-2015q2/bin:$PATH
    * Debian: sudo apt-get install gcc-arm-none-eabi libnewlib-arm-none-eabi
    * arch: pacman -S arm-none-eabi-gcc arm-none-eabi-newlib arm-none-eabi-binutils
* python-yaml
    * Debian/Ubuntu etc.: sudo apt-get install python-yaml
    * Arch: sudo pacman -S python-yaml
    * alternative: via pip/virtualenv
* dfu-util (optional, aber bequemer zum Entwickeln)
    * Debian: sudo apt-get install dfu-util
    * Arch: sudo pacman -S dfu-util

## build firmware

    git clone https://github.com/rad1o/f1rmware.git
    cd f1rmware
    make

## flash rad1o

prerequisite: rad1o is connected to USB

    cd feldtest
    sudo make flash

## MAC OS X Setup:

    ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
    brew tap PX4/homebrew-px4
    brew update
    brew install gcc-arm-none-eabi-49
    brew install dfu-util
    brew install python
    which python # should be /usr/local/bin/python, otherwise change $PATH
    pip install pyaml
    # goto your projects folder
    git clone https://github.com/rad1o/f1rmware.git
    cd f1rmware/
    make
    cd feldtest
    make flash



