#!/bin/sh

current_time=$(date "+%Y-%m-%d_%H%M%S")
echo "Current time $current_time\n"


filenameIR="IR$current_time.jpg"

PARMS='-ex off -ISO 400 -rot 270 -q 100'

raspistill $PARMS $1 -o $filenameIR
