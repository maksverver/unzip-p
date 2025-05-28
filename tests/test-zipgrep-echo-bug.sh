#!/usr/bin/env bash

# Verifies the fix for a bug in zipgrep, where if the script is run by bash,
# then files with names like '-n', 'e', or '-E' cannot be found, because bash
# interprets these as arguments to echo, rather than as strings to echo.

set -eu -o pipefail

ZIPGREP=${1:-../unix/zipgrep}

# Make sure zipgrep uses the compiled version of unzip:
export PATH=..:$PATH
if [ ! -x ../unzip ]; then
    echo "Missing ../unzip"
    exit 1
fi

set -x
"$ZIPGREP"    foo testdata/zipgrep-echo-bug | diff - testdata/zipgrep-echo-bug-output-1.txt
"$ZIPGREP" -l bar testdata/zipgrep-echo-bug | diff - testdata/zipgrep-echo-bug-output-2.txt
