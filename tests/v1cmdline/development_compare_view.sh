#!/usr/bin/env bash
for i in *.am1.log; do
	base=`basename $i .am1.log`
	if [ -e $base.am2.log ]; then
		xxdiff $i $base.am2.log
	fi
done
