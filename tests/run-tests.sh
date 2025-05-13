#!/bin/sh

# Automatically runs all tests in the current directory.
#
# Reports the number of tests that passed and failed, and exits
# with the number of failures as the exit status.

set -eu -o pipefail

basedir=$(dirname $(readlink -f $0))
cd "$basedir"

passed=0
failed=0
for test in test-*.sh; do
    if ./"$test"; then
        echo "$test passed"
        passed=$(expr $passed + 1)
    else
        echo "$test FAILED"
        failed=$(expr $failed + 1)
    fi
done
echo "$(expr $passed + $failed) tests; $passed passed; $failed failed."
exit $failed
