#!/usr/bin/env python

import sys
import os
import mimetypes
import tempfile
import subprocess
import shutil

FNULL = open(os.devnull, 'w')

if len(sys.argv) < 2:
    sys.exit('Usage: %s gif-file' % sys.argv[0])

# Check for dependencies
if subprocess.call(['which', 'gifsicle'], stdout=FNULL, stderr=subprocess.STDOUT):
    sys.exit("This script depends of gifsicle, please install it and make sure is in your env path.")


def get_dependency_script_path(script):
    if not os.access(script, os.X_OK):
        sys.exit(script + " must be within same directory with execution permission")
    return os.path.abspath(script)

img2lcd = get_dependency_script_path('img2lcd.pl')

lcd2ani = get_dependency_script_path('lcd2ani.pl')

oldcwd = os.getcwd()

filename = sys.argv[1]

if os.access(filename, os.R_OK) and mimetypes.guess_type(filename)[0] == 'image/gif':
    absFilePath = os.path.abspath(filename)
else:
    sys.exit('%s must be an image/gif file' % sys.argv[0])

gifDir = os.path.dirname(absFilePath)
# Get gif name without extension
gifName = os.path.splitext(os.path.basename(absFilePath))[0]
# Make temp directory and cd into
tmpDir = tempfile.mkdtemp()
os.chdir(tmpDir)

# Explode GIF in frames
subprocess.call(["gifsicle", "-e", absFilePath])

lcd2aniArgs = []
# Convert each frame in lcd format and build an argument list
for gifFrame in os.listdir(tmpDir):
    subprocess.call([img2lcd, gifFrame, gifFrame + ".lcd"])
    lcd2aniArgs.extend([gifFrame + ".lcd", '200'])

an1File = gifName + ".an1"
# Convert the list of lcd frames into an1 format
subprocess.call([lcd2ani, an1File] + lcd2aniArgs)

# Move the an1 to GIF directory
shutil.copy(an1File, gifDir)

os.chdir(oldcwd)
# Remove temp files
shutil.rmtree(tmpDir)

print "%s -> %s" % (filename, gifDir + '/' + an1File)
