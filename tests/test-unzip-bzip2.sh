#!/bin/sh

# Verifies unzipping files compressed with bzip2 works.

set -eu -o pipefail

UNZIP=${1:-../unzip}

if [ -z "$($UNZIP -v | grep USE_BZIP2)" ]; then
    echo 'bzip2 support not compiled in'
    exit 234  # magic value that means skip this test
fi

TMPDIR=$(mktemp -d)
trap "rm -rf -- $TMPDIR" EXIT

$UNZIP -q -d "$TMPDIR" testdata/lore-ipsum-bzip2.zip
diff -r "$TMPDIR"/lore-ipsum.txt testdata/lore-ipsum.txt
