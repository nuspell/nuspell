#!/usr/bin/env bash

autoreconf -vfi
if [ $? -ne 0 ]; then
	echo 'ERROR: Failed to autoreconfigure nuspell'
	exit 1
fi
./configure CXXFLAGS='-O2 -fno-omit-frame-pointer'
if [ $? -ne 0 ]; then
	echo 'ERROR: Failed to configure nuspell for production'
	exit 1
fi
make -j clean
if [ $? -ne 0 ]; then
	echo 'ERROR: Failed to clean nuspell'
	exit 1
fi
./production-build.sh
