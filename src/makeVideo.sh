#!/bin/bash
# This script uses ffmpeg to convert a gif into a browser friendly video.

# This script was writting by Clemens Scott and tested under Ubuntu 20 LTS
# It us used in production with https://nchrs.xyz

# The repository for this script can be found at
# https://git.sr.ht/~rostiger/anchors/

# The original ffmpeg command was written by Vico Vault
# https://unix.stackexchange.com/questions/40638/how-to-do-i-convert-an-animated-gif-to-an-mp4-or-mv4-on-the-command-line

# Usage: ./makeVideo src dst newWidth(optional)

if [ $1 ]; then
  input=$1
  path=$(dirname $input)        # just/the/path
  name=$(basename $input)       # filename.ext
  base="${name%%.*}"        # filename
  fileExt="${name#*.}"      # .ext
  if [ ! -f $input ]; then
    # check file path validity
    echo "ERROR: attempt to convert gif to mp4 failed."
    echo "${1} isn't a valid file path."
  elif [[ ! $input =~ \.gif$ ]]; then
    # check file type validity
    echo "ERROR: attempt to convert gif to mp4 failed."
    echo ".${fileExt} isn't a valid file type. File extension must be .gif"
  else
    # convert the gif
    dst="${path}/${base}.mp4"  # destination
    origSize=$(ffprobe -v error -select_streams v:0 -show_entries stream=width,height -of csv=s=x:p=0 $input)
    newSize=$origSize
    scale="scale=trunc(iw/2)*2:trunc(ih/2)*2"
    if [ $2 ]; then
      # if a new WIDTH is passed as argument 2
      # the aspect ratio is calculated and used to get the new height
      origW="${origSize%%x*}"
      origH="${origSize#*x}"
      newW=$2
      echo $((200 * $origW/$origH - 100 * $origW/$origH ))
#      newSize="${newW}x${newH}"
#      scale="scale=${newW}:${newH}"
#      echo "Attempting to convert ${name} to mp4 (${origSize} to ${newSize})."
    else 
      echo "Attempting to convert ${name} to mp4 (${origSize})"
    fi
    ffmpeg -i $input -movflags faststart -pix_fmt yuv420p -vf $scale -v error -y $dst
  fi
else
  echo "Please provide a file name and path to the gif you want to convert"
fi
