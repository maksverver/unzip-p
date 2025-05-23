#!/usr/bin/env bash

# Automatically runs all tests in the current directory.
#
# Reports the number of tests that passed and failed, and exits
# with the number of failures as the exit status.

set -eu -o pipefail

basedir=$(dirname $(readlink -f $0))
cd "$basedir"

passed=0
failed=0
skipped=0
disabled=0
for test in test-*.sh; do
    if [ ! -x "$test" ]; then
        echo "$test DISABLED (enable with: chmod a+x $test)"
        disabled=$(expr $disabled + 1)
    elif ./"$test"; then
        echo "$test passed"
        passed=$(expr $passed + 1)
    else
        # Check exit status. 234 is the magic value that means 'skipped'
        status=$?
        if [ $status -eq 234 ]; then
            echo "$test SKIPPED"
            skipped=$(expr $skipped + 1)
        else
            echo "$test FAILED ($status) (disable with: chmod a-x $test)"
            failed=$(expr $failed + 1)
        fi
    fi
done

total=$(expr $passed + $failed + $skipped)
echo "$total tests; $passed passed; $failed failed; $skipped skipped; $disabled disabled."

exit $failed
