#!/bin/sh
set -eu
# GCC: sh build.sh
# Oracle Developer Studio: CC=cc CFLAGS='-xc99=%all -v' sh build.sh
CC=${CC:-gcc}
CFLAGS=${CFLAGS:--std=c99 -Wall -Wextra -pedantic -O2}
for number in 1 2 3 4 5 6 7; do
    "$CC" $CFLAGS -o "z$number" "z$number.c"
done
echo 'Built z1 z2 z3 z4 z5 z6 z7'
