#!/usr/bin/env bash

# Basic smoke test for `unzip -t`

set -eu -o pipefail

UNZIP=${1:-../unzip}

"$UNZIP" -t testdata/unzip60.zip | diff testdata/unzip60-testing.txt -
