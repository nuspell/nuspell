#!/usr/bin/env bash
for i in *aff; do
	if [ `echo $i|grep utf|wc -l` -eq 0 ]; then
		base=`basename $i .aff`
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
	fi
done
