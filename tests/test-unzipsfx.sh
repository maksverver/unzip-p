#!/bin/sh

# Basic smoke test for unzipsfx.

set -eu -o pipefail

UNZIPSFX=${1:-../unzipsfx}

TMPDIR=$(mktemp -d)
trap "rm -rf -- $TMPDIR" EXIT

# Create self-extracting executable by prepending unzipsfx to a zipfile:
cat "$UNZIPSFX" testdata/unzip60.zip > "$TMPDIR"/sfx
chmod a+x "$TMPDIR"/sfx

# Test contents, which should print 'No errors detected'
"$TMPDIR"/sfx -tq | grep -q 'No errors detected'

# Extract contents, and verify they are as expected.
"$TMPDIR"/sfx -qq -d "$TMPDIR"/contents
diff -r "$TMPDIR"/contents testdata/unzip60
