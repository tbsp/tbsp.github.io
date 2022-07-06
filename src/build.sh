#!/bin/bash

# Lint
clang-format -i main.c

# Cleanup
rm -f ./wastingmoves
rm ../*.html

# Linux(debug)
#cc -std=c89 -DDEBUG -Wall -Wno-unknown-pragmas -Wpedantic -Wshadow -Wuninitialized -Wextra -Werror=implicit-int -Werror=incompatible-pointer-types -Werror=int-conversion -Wvla -g -Og -O0 -fsanitize=address -fsanitize=undefined main.c -o nchrs

# Linux(fast)
cc main.c -std=c89 -Os -DNDEBUG -g0 -s -Wall -Wno-unknown-pragmas -o wastingmoves

# RPi
# tcc -Wall main.c -o oscean

# Plan9
# pcc main.c -o oscean

# Valgrind
# gcc -std=c89 -DDEBUG -Wall -Wpedantic -Wshadow -Wuninitialized -Wextra -Werror=implicit-int -Werror=incompatible-pointer-types -Werror=int-conversion -Wvla -g -Og main.c -o oscean
# valgrind ./oscean

# Build Size
 echo "$(du -b ./wastingmoves | cut -f1) bytes written"

# Run
./wastingmoves

# Cleanup
rm -f ./wastingmoves
