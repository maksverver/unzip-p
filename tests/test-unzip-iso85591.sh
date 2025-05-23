#!/usr/bin/env bash

# Verifies unsupported Unicode characters are escaped/replaced,
# while supported characters kept, when using the ISO-8859-1 encoding.

set -eu -o pipefail

UNZIP=${1:-../unzip}

export LC_CTYPE=en_US.iso88591

# Check that we actually have an ISO-8859-1 locale available, otherwise skip.
if ! locale -a | grep -q $LC_CTYPE; then
    exit 234  # magic value that means skip this test
fi

$UNZIP -l testdata/unicode.zip | diff testdata/unicode-listing-iso85591.txt -

$UNZIP -l testdata/iso88591.zip | diff testdata/iso85591-listing-iso85591.txt -
