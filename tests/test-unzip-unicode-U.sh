#!/usr/bin/env bash

# Verifies unicode characters get translated to ASCII in the C locale.
# Note: keep this in sync with test-unzip-unicode-C.sh

set -eu -o pipefail

UNZIP=${1:-../unzip}

TMPDIR=$(mktemp -d)
trap "rm -rf -- $TMPDIR" EXIT

export LC_CTYPE=en_US.UTF-8

# Verify listing is correct
$UNZIP -l testdata/unicode.zip | diff testdata/unicode-listing-U.txt -

# Without Unicode processing, same result since it's already UTF-8
$UNZIP -UU -l testdata/unicode.zip | diff testdata/unicode-listing-U.txt -

# Verify files get extracted to ASCII filenames
$UNZIP -q -d "$TMPDIR" testdata/unicode.zip

ls "$TMPDIR" | diff testdata/unicode-filenames-U.txt -
