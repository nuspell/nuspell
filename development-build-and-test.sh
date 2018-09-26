#!/usr/bin/env bash

cd src/nuspell
./clang-format.sh
cd ../../tests
./clang-format.sh
cd ..
make -j clean
make -j
if [ $? -eq 0 ]; then
	make -j check
else
	echo FAILED
	exit $?
fi
