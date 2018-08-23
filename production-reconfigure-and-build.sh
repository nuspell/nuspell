#!/usr/bin/env bash

autoreconf -vfi
if [ $? -ne 0 ]; then
	echo 'ERROR: Failed to autoreconfigure nuspell'
	exit 1
fi
./configure CXXFLAGS='-O2 -fno-omit-frame-pointer'
if [ $? -ne 0 ]; then
	echo 'ERROR: Failed to configure nuspell'
	exit 1
fi
make clean
if [ $? -ne 0 ]; then
	echo 'ERROR: Failed to clean nuspell'
	exit 1
fi
./production-build.sh
