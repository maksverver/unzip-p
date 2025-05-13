#!/bin/sh

# Basic smoke test for `unzip -l`

set -eu -o pipefail

UNZIP=${1:-../unzip}

"$UNZIP" -l testdata/unzip60.zip | diff testdata/unzip60-listing.txt -

"$UNZIP" -l -v testdata/unzip60.zip | diff testdata/unzip60-listing-verbose.txt -
