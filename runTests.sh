#!/bin/bash

# prints failed tests
# get qqwing at http://qqwing.com/download.html

CORES=1
QQWING=1

printHelp() {
    echo "usage: runTests.sh [OPTIONS] [FILE]

  -s          Run tests sequentially, not in parallel. Default is parallel.
  -J CORES    Set the number of cores to use for parallel testing. Default is 1.
  -x          Run without checking against qqwing. Return failures for solutions including a zero.

This will output any lines from FILE for which qqwing's solution differs from sudoku.exe's."
    exit
}

# The workhorse function. Compare solutions or look for unsolved cells
compareSolutions() {
    if [ -n "$1" ]; then
        MYSOLV=$(./sudoku.exe <<< "$1" | tr -d ' ')
        if [ "$QQWING" == 1 ]; then
            THEIRSOLV=$(qqwing --solve --compact <<< "$1")
            if [ "$MYSOLV" != "$THEIRSOLV" ]
            then
                echo "$1" | tr -d ' '
            fi
        else
            if [ -n "$(echo "$MYSOLV" | grep 0)" ]; then
                echo "$1" | tr -d ' '
            fi
        fi
    fi
}
export -f compareSolutions

# flag handling
while getopts hsxJ:: FLAGNAME
do
    case "$FLAGNAME" in
        h)"printHelp";;
        s)SEQUENTIAL=1;;
        J)CORES="$OPTARG";;
        x)QQWING=0;;
        *)echo "Invalid arg";;
    esac
done

shift $(($OPTIND -1))

# no filename was given
if [ -z "$1" ]; then
    echo "runTests.sh: missing operand

Try \`runTests.sh -h' for more information."
    exit 1
fi

# Handle missing commands
if [ -z "$(command -v ./sudoku.exe)" ]; then
    echo "sudoku.exe not found, did you run make?"
    exit 1;
fi

if [ -z "$(command -v qqwing)" ]; then
    echo "qqwing not found, using -x (see -h for help)..."
    QQWING=0
fi

# run the tests in parallel or sequentially
export QQWING
if [ -n "$SEQUENTIAL" ]; then
    while read LINE; do
        compareSolutions "$LINE"
    done <"$1"
else
    # Prefer gnu-parallel, but if it's not installed use xargs
    if [ -n "$(command -v parallel)" ]; then
        parallel -d '\n' -j "$CORES" -a "$1" compareSolutions
    else
        echo "GNU-parallel not found, using xargs instead..."
        xargs -d '\n' -n 1 -P "$CORES" -a "$1" bash -c 'compareSolutions "$@"' _
    fi
fi
