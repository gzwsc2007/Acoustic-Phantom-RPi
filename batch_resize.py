#!/usr/bin/env python
# Batch thumbnail generation script using PIL

import sys
import os.path
from PIL import Image
from os import listdir
from os.path import isfile, join

mypath = sys.argv[1]
print mypath
files = [f for f in listdir(mypath) if isfile(join(mypath, f))]

thumbnail_size = (28, 28)

# Loop through all provided arguments
for i in range(1, len(files)):
    try:
        # Attempt to open an image file
        filepath = files[i]
        image = Image.open(filepath)
    except IOError, e:
        # Report error, and then skip to the next argument
        print "Problem opening", filepath, ":", e
        continue

    # Resize the image
    image = image.resize(thumbnail_size, Image.ANTIALIAS)
    
    # Split our original filename into name and extension
    (name, extension) = os.path.splitext(filepath)
    
    # Save the thumbnail as "(original_name)_thumb.png"
    image.save(name + '_thumb.png')