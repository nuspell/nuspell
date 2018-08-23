#!/usr/bin/env bash

make -j CXXFLAGS='-O2 -fno-omit-frame-pointer'
if [ $? -ne 0 ]; then
	echo 'ERROR: Failed to build nuspell'
	exit 1
fi
