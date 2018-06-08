#!/usr/bin/env bash

autoreconf -vfi
if [ $? -eq 0 ]; then
	./configure --with-warnings CXXFLAGS='-g -O0 -Wall -Wextra'
else
	echo FAILED
	exit $?
fi
