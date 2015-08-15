#! /bin/bash
import sys
import binascii
import struct

basename = 'rgb_leds'

try:
    filename = 'assets/' + basename + '.led'
    delay = int(sys.argv[1])
except:
    sys.stderr.write('Usage: convertleds.py <delay>\n')
    sys.exit(1)

output = ''

with open(filename) as fp:
    contents = fp.read()
    contents = contents.replace(chr(10), '')
    contents = contents.replace(' ', '')
    newname = 'files/' + basename + '.hex'
    with open(newname, 'w') as fpW:
        fpW.write(struct.pack('>H', delay))
        fpW.write(binascii.unhexlify(contents))
