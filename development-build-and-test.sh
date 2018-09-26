#!/usr/bin/env bash

cd src/nuspell
./clang-format.sh
cd ../../tests
./clang-format.sh
cd ..
make -j
if [ $? -ne 0 ]; then
	echo 'ERROR: Failed to build nuspell for development'
	exit 1
fi
make -j check
if [ $? -ne 0 ]; then
	echo 'ERROR: Failed to test nuspell for development'
	exit 1
fi
