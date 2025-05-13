#!/bin/sh

# Verifies a basic zip64 file can be extracted.
# (Note this doesn't work with funzip currently!)

set -eu -o pipefail

UNZIP=${1:-../unzip}

TMPDIR=$(mktemp -d)
trap "rm -rf -- $TMPDIR" EXIT

unzip -q -d "$TMPDIR" testdata/zip64.zip
diff -r "$TMPDIR"/hello.txt testdata/hello.txt
diff -r "$TMPDIR"/lore-ipsum.txt testdata/lore-ipsum.txt
