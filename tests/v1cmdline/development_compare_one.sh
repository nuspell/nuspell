#!/usr/bin/env bash
if [ $# -ne 1 ]; then
	echo 'ERROR: Missing argument'
	exit
fi
i=$1.aff
if [ `echo $i|grep utf|wc -l` -eq 0 ]; then
	base=`basename $i .aff`
	cd ../..
	make
	if [ $? -ne 0 ]; then
		exit
	fi
	cd tests/v1cmdline
	if [ -e $base.good ]; then
		if [ -e $base.wrong ]; then
			# don't use src/tools/.libs/hunspell here
			../../src/tools/hunspell -d $base -a $base.good $base.wrong
			../../src/nuspell/nuspell -d $base $base.good $base.wrong
		else
			# don't use src/tools/.libs/hunspell here
			../../src/tools/hunspell -d $base -a $base.good
			../../src/nuspell/nuspell -d $base $base.good
		fi
	else
		if [ -e $base.wrong ]; then
			# don't use src/tools/.libs/hunspell here
			../../src/tools/hunspell -d $base -a $base.wrong
			../../src/nuspell/nuspell -d $base $base.wrong
		fi
	fi
	xxdiff $i.am1.log $i.am2.log
fi
