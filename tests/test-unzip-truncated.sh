#!/usr/bin/env bash

# Verifies that unzip fails with status code 9
# when the central directory is not found.

UNZIP=${1:-../unzip}

set -eu -o pipefail

"$UNZIP" -t testdata/truncated 2> /dev/null \
    | diff testdata/truncated-testing.txt - \
    && status=0 || status=$?
if [ $status -ne 9 ]; then
    echo "Incorrect status code: $status"
    exit 1
fi
