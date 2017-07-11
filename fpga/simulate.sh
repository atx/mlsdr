#! /bin/bash

BENCH=$1

if [ ! -e "$BENCH" ]; then
	echo "$BENCH does not exist!"
	exit 1
fi

VSRCS=$(echo *.v | tr ' ' '\n' | grep -v '.*_tb.v' | grep -v '\(pll\|toplevel\|fifo\|ft232\)')
VSRCS="$BENCH $VSRCS"
OUT="${BENCH%.*}.sim"

iverilog -o $OUT $VSRCS
vvp $OUT -vcd
