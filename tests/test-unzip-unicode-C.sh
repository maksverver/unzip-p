#!/bin/sh

# Verifies unicode characters get translated to ASCII in the C locale.
# Note: keep this in sync with test-unzip-unicode-U.sh

set -eu -o pipefail

UNZIP=${1:-../unzip}

TMPDIR=$(mktemp -d)
trap "rm -rf -- $TMPDIR" EXIT

export LC_CTYPE=C

# Verify listing is correct: UTF-8 sequences are replaced with #U00f8 etc.
$UNZIP -l testdata/unicode.zip | diff testdata/unicode-listing-C.txt -

# Without Unicode processing: UTF-8 bytes are replaced with '?'
$UNZIP -UU -l testdata/unicode.zip | diff testdata/unicode-listing-C-UU.txt -

# Verify files get extracted to ASCII filenames
$UNZIP -q -d "$TMPDIR" testdata/unicode.zip

ls "$TMPDIR" | diff testdata/unicode-filenames-C.txt -
