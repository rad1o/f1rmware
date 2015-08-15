#! /usr/bin/env python
import sys
import binascii
import struct

try:
    basename = sys.argv[1]
    filename = 'assets/' + basename + '.led'
    delay = int(sys.argv[2])
except:
    sys.stderr.write('Usage: convertleds.py <name> <delay>\n')
    sys.exit(1)

output = ''

with open(filename) as fp:
    contents = fp.read()
    contents = contents.replace(chr(10), '')
    contents = contents.replace(' ', '')
    newname = 'files/' + basename + '.l3d'
    with open(newname, 'w') as fpW:
        fpW.write(struct.pack('>H', delay))
        fpW.write(binascii.unhexlify(contents))
