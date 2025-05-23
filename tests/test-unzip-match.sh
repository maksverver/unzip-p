#!/usr/bin/env bash

# Tests file pattern matching with wildcards (? and *) and bracket groups ([]),
# and the options -W (stop at directory) and -C (case insensitive).
#
# If this testcase fails, `set -x` is helpful for debugging.

set -eu -o pipefail

UNZIP=${1:-../unzip}
TMPFIL=$(mktemp)
trap "rm -f -- $TMPFIL" EXIT

# No arguments: match all files
cat >$TMPFIL <<EOF
Archive:  testdata/cases.zip
  Length      Date    Time    Name
---------  ---------- -----   ----
        0  2025-05-15 17:49   foo/
        0  2025-05-15 17:49   foo/baz
        0  2025-05-15 17:49   foo/BAZ
        0  2025-05-15 17:49   foo/BAR/
        0  2025-05-15 17:49   foo/BAR/quux
        0  2025-05-15 17:49   foo/BAR/QUUX
        0  2025-05-15 17:49   foo/bar/
        0  2025-05-15 17:49   foo/bar/quux
        0  2025-05-15 17:49   foo/bar/QUUX
        0  2025-05-15 17:49   FOO/
        0  2025-05-15 17:49   FOO/baz
        0  2025-05-15 17:49   FOO/BAZ
        0  2025-05-15 17:49   FOO/BAR/
        0  2025-05-15 17:49   FOO/BAR/quux
        0  2025-05-15 17:49   FOO/BAR/QUUX
        0  2025-05-15 17:49   FOO/bar/
        0  2025-05-15 17:49   FOO/bar/quux
        0  2025-05-15 17:49   FOO/bar/QUUX
---------                     -------
        0                     18 files
EOF
$UNZIP -l testdata/cases.zip | diff - $TMPFIL

# Match specific files
cat >$TMPFIL <<EOF
Archive:  testdata/cases.zip
  Length      Date    Time    Name
---------  ---------- -----   ----
        0  2025-05-15 17:49   foo/BAR/quux
        0  2025-05-15 17:49   foo/bar/QUUX
        0  2025-05-15 17:49   FOO/bar/
---------                     -------
        0                     3 files
EOF
$UNZIP -l testdata/cases.zip FOO/bar/ foo/bar/QUUX foo/BAR/quux | diff - $TMPFIL

# Multiple character wildcard (case sensitive)
cat >$TMPFIL <<EOF
Archive:  testdata/cases.zip
  Length      Date    Time    Name
---------  ---------- -----   ----
        0  2025-05-15 17:49   foo/baz
        0  2025-05-15 17:49   foo/bar/
        0  2025-05-15 17:49   foo/bar/quux
        0  2025-05-15 17:49   foo/bar/QUUX
---------                     -------
        0                     4 files
EOF
$UNZIP -l    testdata/cases.zip 'foo/b*'  | diff - $TMPFIL

# Double wildcard counteracts stop at dir:
$UNZIP -l -W testdata/cases.zip 'foo/b**' | diff - $TMPFIL

# Multiple character wildcard (stop at dir)
cat >$TMPFIL <<EOF
Archive:  testdata/cases.zip
  Length      Date    Time    Name
---------  ---------- -----   ----
        0  2025-05-15 17:49   foo/baz
---------                     -------
        0                     1 file
EOF
$UNZIP -W -l testdata/cases.zip 'foo/b*' | diff - $TMPFIL

# Multiple character wildcard (stop at dir, case insensitive)
cat >$TMPFIL <<EOF
Archive:  testdata/cases.zip
  Length      Date    Time    Name
---------  ---------- -----   ----
        0  2025-05-15 17:49   foo/baz
        0  2025-05-15 17:49   foo/BAZ
        0  2025-05-15 17:49   FOO/baz
        0  2025-05-15 17:49   FOO/BAZ
---------                     -------
        0                     4 files
EOF
$UNZIP -l -C -W testdata/cases.zip 'foo/b*' | diff - $TMPFIL

# Single character wildcard
cat >$TMPFIL <<EOF
Archive:  testdata/cases.zip
  Length      Date    Time    Name
---------  ---------- -----   ----
        0  2025-05-15 17:49   foo/BAR/
        0  2025-05-15 17:49   foo/bar/
---------                     -------
        0                     2 files
EOF
$UNZIP -l testdata/cases.zip 'foo/????' | diff - $TMPFIL

# Single character wildcard (stop at dir)
cat >$TMPFIL <<EOF
Archive:  testdata/cases.zip
  Length      Date    Time    Name
---------  ---------- -----   ----
---------                     -------
        0                     0 files
EOF
if $UNZIP -l -W testdata/cases.zip 'foo/????'; then
  exit 1  # expected to fail, but did not!
fi | diff - $TMPFIL

# Bracket group
cat >$TMPFIL <<EOF
Archive:  testdata/cases.zip
  Length      Date    Time    Name
---------  ---------- -----   ----
        0  2025-05-15 17:49   foo/BAR/quux
---------                     -------
        0                     1 file
EOF
$UNZIP -l -W testdata/cases.zip 'foo[/]BA[ZRX]/[^p]uux' | diff - $TMPFIL

# Bracket group (case insensitive)
cat >$TMPFIL <<EOF
Archive:  testdata/cases.zip
  Length      Date    Time    Name
---------  ---------- -----   ----
        0  2025-05-15 17:49   foo/BAR/quux
        0  2025-05-15 17:49   foo/BAR/QUUX
        0  2025-05-15 17:49   foo/bar/quux
        0  2025-05-15 17:49   foo/bar/QUUX
        0  2025-05-15 17:49   FOO/BAR/quux
        0  2025-05-15 17:49   FOO/BAR/QUUX
        0  2025-05-15 17:49   FOO/bar/quux
        0  2025-05-15 17:49   FOO/bar/QUUX
---------                     -------
        0                     8 files
EOF
$UNZIP -l -C -W testdata/cases.zip 'foo[/]BA[ZRX]/[^p]uux' | diff - $TMPFIL

# Weird edge case: with -W, an even number of consectuive asterisks matches
# an arbitrary string (as expected, this is equivalent to just "**"), but
# an odd number of consecutive asterisks (three or more) cannot match any
# substring containing '/', even though it should. This is probably due to the
# peculiar implementation of recmatch() in match.c which can abort the search
# early by returning 2, but I didn't investigate it in detail.

cat >$TMPFIL <<EOF
Archive:  testdata/cases.zip
  Length      Date    Time    Name
---------  ---------- -----   ----
        0  2025-05-15 17:49   foo/BAR/quux
        0  2025-05-15 17:49   foo/bar/quux
---------                     -------
        0                     2 files
EOF
$UNZIP -l -W testdata/cases.zip 'f****x' | diff - $TMPFIL

cat >$TMPFIL <<EOF
Archive:  testdata/cases.zip
  Length      Date    Time    Name
---------  ---------- -----   ----
---------                     -------
        0                     0 files
EOF
if $UNZIP -l -W testdata/cases.zip 'f***x'; then
  exit 1  # expected to fail, but did not!
fi | diff - $TMPFIL
