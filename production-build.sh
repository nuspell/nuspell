#!/usr/bin/env bash

make -j
if [ $? -ne 0 ]; then
	echo 'ERROR: Failed to build nuspell for production'
	exit 1
fi
