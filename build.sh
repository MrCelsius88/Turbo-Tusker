#!/bin/sh

rm -r build
mkdir -p build
pushd build

gcc -DTUSKER_LINUX=1 -g ../src/tusker_linux_x11.c -lX11 -lasound -lGL -lm -o TurboTusker

popd
