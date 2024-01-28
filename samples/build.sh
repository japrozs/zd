#!/bin/sh

set -xe
clang hello.c -c -o hello.o

# linking stage
# ld hello.o -o hello -l System -syslibroot (xcrun -sdk macosx --show-sdk-path) -arch arm64
rm -rf out/*.dSYM