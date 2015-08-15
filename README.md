![Build Status](https://travis-ci.org/rad1o/f1rmware.svg)

Build Log: https://travis-ci.org/rad1o/f1rmware

# rad1o

This is the f1rmware of the CCCamp 2015 radio badge.

## Project Infrastructure
Website: http://rad1o.badge.events.ccc.de/software

IRC channel: irc://irc.darkfasel.net/#rad1o (Port 6697 oder 9999, TLS-only, IPv6 Support)
 * Web-IRC-Client: https://webirc.darkfasel.net/#rad1o
 * other contact information:  https://rad1o.badge.events.ccc.de/contact


[Build instructions](doc/build.md)

[Some notes on toggling LEDs](doc/debugging.md)


## instructions in how to create your own loadables and update them to the rad1o

add your .c file to f1rmware/l0adables

add the n1c and c1d to the Makefile in f1rmware/l0adables

run make in f1rmware/l0adables

run cp [name of your file].c1d /[location of your mounted rad1o]/

unmount the rad1o

restart the device




If you get an error of incompatible build numbers, you propably need to recompile the whole project in the f1rmware (run make) 
