#!/bin/sh
set -eu
: "${CC:=cc}"
"$CC" -std=c99 -Wall -Wextra -Werror -pedantic number_bases.c test_number_bases.c -o number_bases_test
./number_bases_test
