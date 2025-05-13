#!/bin/sh

# Verifies that unzip fails with status code 2 when a CRC fails.

UNZIP=${1:-../unzip}

set -eu -o pipefail

"$UNZIP" -t testdata/badcrc 2> /dev/null \
    | diff testdata/badcrc-testing.txt - \
    && status=0 || status=$?
if [ $status -ne 2 ]; then
    echo "Incorrect status code: $status"
    exit 1
fi
