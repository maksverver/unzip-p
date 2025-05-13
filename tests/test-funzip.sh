#!/bin/sh

# Basic smoke test for funzip

set -eu -o pipefail

FUNZIP=${1:-../funzip}

# funzip <file> should extract the first entry in the zip file,
# and write its content to stdout:
"$FUNZIP" testdata/lore-ipsum.zip | diff testdata/lore-ipsum.txt -

# without a file argument, funzip reads from a pipe:
cat testdata/lore-ipsum.zip | "$FUNZIP" | diff testdata/lore-ipsum.txt -
