#!/bin/sh

# Basic smoke test for zipinfo.

set -e -E -o pipefail

ZIPINFO=${1:-../zipinfo}

"$ZIPINFO" testdata/unzip60.zip | diff testdata/unzip60-zipinfo.txt -

"$ZIPINFO" -v testdata/unzip60.zip | diff testdata/unzip60-zipinfo-verbose.txt -
