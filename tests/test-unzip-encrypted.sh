#!/usr/bin/env bash

# Verifies `unzip` can extract a password-protected archive.

set -eu -o pipefail

UNZIP=${1:-../unzip}

TMPDIR=$(mktemp -d)
trap "rm -rf -- $TMPDIR" EXIT

password='squeamish ossifrage'
$UNZIP -q -d "$TMPDIR" -P "$password" testdata/encrypted.zip
diff "$TMPDIR"/hello.txt testdata/hello.txt
