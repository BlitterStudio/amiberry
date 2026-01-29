#!/bin/sh
FAILURE=99
SUCCESS=0

expect_failure=0
if [ "$1" = "--expect-failure" ]; then
    expect_failure=1
    shift
fi

valgrind --error-exitcode="$FAILURE" --log-file="valgrind.log" "$@"
exitcode=$?
# Report the log messages just in case there was anything interesting
cat "valgrind.log" >&2

if [ "$exitcode" -eq "$SUCCESS" ]; then
    # Valgrind didn't reject our code, but if it warned
    # about stack-switching, we care about that too.
    # None of our tests should use enough stack
    # to trigger this by accident.
    if grep -qlF "Warning: client switching stacks?" "valgrind.log"; then
        echo "Test confused Valgrind by switching stacks" >&2
        exitcode=$FAILURE
    fi
fi

if [ "$expect_failure" -eq 1 ]; then
    if [ "$exitcode" -eq "$SUCCESS" ]; then
        echo "Test passed, but was expected to fail?" >&2
        exitcode=$FAILURE
    else
        echo "Task failed as expected" >&2
        exitcode=$SUCCESS
    fi
fi

exit "$exitcode"
