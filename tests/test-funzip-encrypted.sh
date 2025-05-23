#!/usr/bin/env bash

# Verifies `funzip` can extract a password-protected archive.

set -eu -o pipefail

FUNZIP=${1:-../funzip}

password='squeamish ossifrage'
"$FUNZIP" -"$password" testdata/encrypted.zip | diff testdata/hello.txt -
