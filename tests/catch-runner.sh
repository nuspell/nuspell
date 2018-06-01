#!/bin/sh
T="$1"
shift
"$T" -r tap "$@"
