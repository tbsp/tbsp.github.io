#!/bin/bash
# This script uses imagemagick to convert an image into a cover image with a 3:2 ratio.

# This script was writting by Clemens Scott and tested under Ubuntu 20 LTS
# It us used in production with https://nchrs.xyz

# The repository for this script can be found at
# https://git.sr.ht/~rostiger/anchors/

FILE=$1
OFFSETX=${2:-0}
OFFSETY=${3:-0}
PERCENTAGE=${4:-100}

if [ $FILE ]; then
  convert $FILE -gravity center -strip -auto-orient -crop 3:2+$OFFSETX+$OFFSETY +repage -crop $PERCENTAGE% +repage -quality 95 cover.jpg
  eog cover.jpg
else
  echo "Please provide a file name."
fi
