#!/bin/sh

# Verifies unzip can extract a subset of files given on the command line.

set -eu -o pipefail

UNZIP=${1:-../unzip}

TMPDIR=$(mktemp -d)
trap "rm -rf -- $TMPDIR" EXIT

FILES="unzip60/beos/beos.c unzip60/README unzip60/zip.h"

unzip -q -d "$TMPDIR" testdata/unzip60.zip $FILES
for file in $FILES; do
    diff "$TMPDIR"/$file testdata/unzip60/$file
done
