#!/bin/sh

rm -r build
mkdir -p build
pushd build

# CODE DIRECTORY
code=../src

# COMPILER FLAGS
compiler="-m32 -Wall -Werror" # Architecture compile mode | Display all warnings | Display all warnings as errors

# WARNINGS TO IGNORE
ignored_warnings="-Wno-parentheses -Wno-unused-variable -Wno-unused-function -Wno-format-overflow"

# DEBUG VARIABLES
debug="-g" # Generate debug info

# LINKER OPTIONS
linker_opts="-static-libgcc" # Statically link to the CRT

# LINUX PLATFORM LIBRARIES
linux_libs="-lX11 -lGL -lm"

# CROSS-PLATFORM DEFINES
defines="-DTUSKER_SLOW=1 -DTUSKER_INTERNAL=1"

gcc -O0 $compiler $ignored_warnings -DTUSKER_LINUX=1 $defines $debug $code/tusker_linux_x11.c $linker_opts $linux_libs -o TurboTusker

popd
