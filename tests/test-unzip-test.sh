#!/bin/sh

# Basic smoke test for `unzip -t`

set -e -E -o pipefail

UNZIP=${1:-../unzip}

"$UNZIP" -t testdata/unzip60.zip | diff testdata/unzip60-testing.txt -
