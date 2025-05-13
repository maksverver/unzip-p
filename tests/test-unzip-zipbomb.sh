#!/bin/sh

# Verifies that zipbomb detection in `unzip` is effective.
#
# Zip bomb vulnerability:
# https://nvd.nist.gov/vuln/detail/CVE-2019-13232
#
# zbsm.zip is from "A better zipbomb" by David Fifield:
# https://www.bamsoftware.com/hacks/zipbomb/

set -o pipefail

UNZIP=${1:-../unzip}

"$UNZIP" -qt testdata/zbsm.zip 0 1 2>&1 | diff testdata/zipbomb-error.txt -
status=$?
if [ $status -ne 12 ]; then
    echo "Incorrect status code: $status"
    exit 1
fi

set +e
UNZIP_DISABLE_ZIPBOMB_DETECTION=TRUE "$UNZIP" -qt testdata/zbsm.zip 0 1 \
    | grep -q 'No errors detected'
