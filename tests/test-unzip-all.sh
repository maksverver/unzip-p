#!/bin/sh

# Basic smoke test for unzip.
#
# Simply extracts all files into a temporary directory and then
# verifies the contents are as expected.

set -e -E -o pipefail

UNZIP=${1:-../unzip}

TMPDIR=$(mktemp -d)
trap "rm -rf -- $TMPDIR" EXIT

unzip -q -d "$TMPDIR" testdata/unzip60.zip
diff -r "$TMPDIR" testdata/unzip60
