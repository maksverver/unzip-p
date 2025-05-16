#!/bin/sh

# Verifies unicode characters get translated to ASCII in the C locale.
# Note: keep this in sync with test-unzip-unicode-U.sh

set -eu -o pipefail

UNZIP=${1:-../unzip}

TMPDIR=$(mktemp -d)
trap "rm -rf -- $TMPDIR" EXIT

export LC_CTYPE=C

# Verify listing is correct
$UNZIP -l testdata/unicode.zip | diff testdata/unicode-listing-C.txt -

# Verify files get extracted to ASCII filenames
$UNZIP -q -d "$TMPDIR" testdata/unicode.zip

ls "$TMPDIR" | diff testdata/unicode-filenames-C.txt -
