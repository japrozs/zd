#!/bin/sh

set -xe
clang -Wall -Wextra -pedantic main.c -o out/zd
rm -rf out/*.dSYM