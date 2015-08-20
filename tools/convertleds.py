#! /usr/bin/env python2
import sys
import binascii
import struct
import getopt
import re

def print_usage():
    sys.stderr.write('Usage:\n')
    sys.stderr.write('\tconvertleds.py [-d delay] input.led [output.l3d]\n')

options, remainder = getopt.getopt(sys.argv[1:], 'd:o:h', ['delay=', 'outdir=', 'help'])

delay_arg = None
delay_file = None
default_delay = 50
delay_regex = "^delay:(.+)\n"

outdir = "."

for opt, arg in options:
    if opt in ('-d', '--delay'):
        delay_arg = int(arg)
    if opt in ('-o', '--outdir'):
        outdir = arg
    if opt in ('-h', '--help'):
        print_usage()
        sys.exit(0)

if len(remainder) == 0:
    sys.stderr.write('missing input file\n')
    print_usage()
    sys.exit(1)

elif len(remainder) > 2:
    sys.stderr.write('too many arguments\n')
    print_usage()
    sys.exit(1)

filename = remainder[0]

if len(remainder) == 1:
    outfilename = re.sub('\.[^.]*$','.l3d',filename)
    outfilename = outdir + "/" + re.sub('.*/','',outfilename)
    
else:
    outfilename = remainder[1]

with open(filename) as fp:
    contents = fp.read()

    delay_match = re.search(delay_regex, contents)
    if delay_match:
        delay_file = delay_match.group(1)
        contents = re.sub(delay_regex, '', contents)

    # first use argument if available, then use delay from file if available, then fallback to default
    delay = delay_arg if delay_arg else int(delay_file) if delay_file else default_delay

    lines = len(filter(lambda x: x.strip(), contents.splitlines()))

    if lines > 50:
        sys.stderr.write('ERROR: failed to convert %s\n' % filename)
        sys.stderr.write('currently only animations with a maximum of 50 frames are supported!\n')
        sys.exit(2)

    elif lines == 1:
        delay = 65535 # for static files

    contents = contents.replace(chr(10), '')
    contents = contents.replace(' ', '')
    with open(outfilename, 'w') as fpW:
        fpW.write(struct.pack('>H', delay))
        fpW.write(binascii.unhexlify(contents))
