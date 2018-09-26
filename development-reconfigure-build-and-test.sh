#!/usr/bin/env bash

autoreconf -vfi
if [ $? -eq 0 ]; then
	./configure CXXFLAGS='-g -O0 -Wall -Wextra -Werror'
else
	echo FAILED
	exit $?
fi
./development-build-and-test.sh
