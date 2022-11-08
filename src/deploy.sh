#!/bin/bash
./batchVariants
./build.sh
#echo "Uploading to webserver..."
#rsync -rvhP --inplace  --include=".htaccess" --exclude={'.*','src'} $HOME/projects/c/nchrs/ uberspace:html/
#./upload.sh