#!/usr/bin/env python
# vim: set ts=4 sw=4 tw=0 et pm=:
import struct
import sys
import os.path
import getopt
import zlib

options, remainder = getopt.getopt(sys.argv[1:], 'o:w:c:r:s:v', ['offset=',
														'window=',
														'center=',
														'rate=',
														'search-depth=',
														'verbose',
														])
center = None
sample_rate = None
symbols_per_second = 25000
preamble_length = 64
search_offset = None
search_window = None
search_depth = 0.007
verbose = False

for opt, arg in options:
	if opt in ('-o', '--search-offset'):
		search_offset = int(arg)
	elif opt in ('-v', '--verbose'):
		verbose = True

if len(remainder)<1:
	in_file = "/dev/stdin"
else:
	in_file = remainder[0]

#in = open(in_file,"rb")

if len(remainder)<2:
	out = open("/dev/stdout","wb")
else:
	out = open(remainder[1],"wb")

# ref. NXP UM10503 Table 24 (Boot image header description)
header = ""
header += struct.pack ('<B',int("11"+"011010",2)) # AES enc not active + No hash active
header += struct.pack ('<B',int("11"+"111111",2)) # RESERVED + AES_CONTROL
size=os.path.getsize(in_file)
size=(size+511)/512 # 512 byte blocks, rounded up
header += struct.pack('<H',size)                  # (badly named) HASH_SIZE
header += struct.pack('8B',*[0xff] *8)            # HASH_VALUE (unused)
header += struct.pack('4B',*[0xff] *4)            # RESERVED

out.write( header )

infile=open(in_file,"rb").read()
out.write( infile )

suffix= ""
suffix+= struct.pack('<H', 0x0000)  # bcdDevice
suffix+= struct.pack('<H', 0x000c)  # idProduct
suffix+= struct.pack('<H', 0x1fc9)  # idVendor
suffix+= struct.pack('<H', 0x0100)  # bcdDFU
suffix+= b'DFU'[::-1]               # (reverse DFU)
suffix+= struct.pack('<B', 16)      # suffix length

out.write( suffix )

checksum=zlib.crc32(header+infile+suffix)
checksum=checksum^0xffffffff

if checksum<0:
    checksum+=pow(2,32);

out.write( struct.pack('I', checksum) ) # crc32

