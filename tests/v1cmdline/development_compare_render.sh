#!/usr/bin/env bash
for i in *.am?.gv; do
	if [ `wc -l $i|awk '{print $1}'` -ne 3 ]; then
		dot -Tsvg -o`basename $i gv`svg $i
	fi
done
