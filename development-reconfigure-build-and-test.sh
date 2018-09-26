#!/usr/bin/env bash

autoreconf -vfi
if [ $? -ne 0 ]; then
	echo 'ERROR: Failed to autoreconfigure nuspell'
	exit 1
fi
./configure CXXFLAGS='-g -O0 -Wall -Wextra -Werror'
if [ $? -ne 0 ]; then
	echo 'ERROR: Failed to configure nuspell for development'
	exit 1
fi
make -j clean
if [ $? -ne 0 ]; then
	echo 'ERROR: Failed to clean nuspell'
	exit 1
fi
./development-build-and-test.sh
