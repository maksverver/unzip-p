#!/usr/bin/env bash

# Smoke test for basic functionality of zipgrep.
#
# This includes zipgrep -l (print matching file names only)
# and zipgrep -h (output matches without file names)

set -eu -o pipefail

ZIPGREP=${1:-../unix/zipgrep}

# Make sure zipgrep uses the compiled version of unzip:
export PATH=..:$PATH
if [ ! -x ../unzip ]; then
    echo "Missing ../unzip"
    exit 1
fi

"$ZIPGREP" -i 'O+ps' testdata/unzip60.zip | diff testdata/unzip60-zipgrep.txt -

"$ZIPGREP" -l -i 'O+ps' testdata/unzip60.zip | diff testdata/unzip60-zipgrep-l.txt -

"$ZIPGREP" -h -i 'O+ps' testdata/unzip60.zip | diff testdata/unzip60-zipgrep-h.txt -
