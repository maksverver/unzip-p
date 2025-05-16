#!/bin/sh
#
# Verifies the backup option (-B) works as advertised.
#
# When this option is set, if a file to be extracted already exists,
# the old file is renamed to preserve it.

set -eu -o pipefail

UNZIP=${1:-../unzip}

TMPDIR=$(mktemp -d)
trap "rm -rf -- $TMPDIR" EXIT

cp testdata/hello.txt "$TMPDIR"/lore-ipsum.txt

$UNZIP -B -q -d "$TMPDIR" testdata/lore-ipsum.zip
$UNZIP -B -q -d "$TMPDIR" testdata/lore-ipsum.zip
$UNZIP -B -q -d "$TMPDIR" testdata/lore-ipsum.zip

diff "$TMPDIR"/lore-ipsum.txt   testdata/lore-ipsum.txt
diff "$TMPDIR"/lore-ipsum.txt~  testdata/hello.txt
diff "$TMPDIR"/lore-ipsum.txt~1 testdata/lore-ipsum.txt
diff "$TMPDIR"/lore-ipsum.txt~2 testdata/lore-ipsum.txt
! test -e "$TMPDIR"/lore-ipsum.txt~4
