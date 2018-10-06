#!/usr/bin/env bash

autoreconf -vfi
if [ $? -ne 0 ]; then
	echo 'ERROR: Failed to autoreconfigure nuspell'
	exit 1
fi
./configure --enable-code-coverage
if [ $? -ne 0 ]; then
	echo 'ERROR: Failed to configure nuspell for development'
	exit 1
fi
make -j clean
if [ $? -ne 0 ]; then
	echo 'ERROR: Failed to clean nuspell'
	exit 1
fi
make -j
make -j check-code-coverage CODE_COVERAGE_LCOV_OPTIONS=--no-external \
        CODE_COVERAGE_GENHTML_OPTIONS=--prefix' $(abs_top_builddir)' \
        CODE_COVERAGE_IGNORE_PATTERN='"*/tests/*"'
if [ $? -ne 0 ]; then
	echo 'ERROR: Failed to report code coverage nuspell'
	exit 1
fi
