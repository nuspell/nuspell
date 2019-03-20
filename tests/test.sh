#!/bin/bash
# Copyright 2016-2019 Dimitrij Mijoski
# Copyright (C) 2002-2017 Németh László
#
# This file is part of Nuspell.
#
# Nuspell is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Nuspell is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with Nuspell.  If not, see <http://www.gnu.org/licenses/>.

function check_valgrind_log () {
if [[ "$VALGRIND" != "" && -f $temp_dir/test.pid* ]]; then
	log=$(ls $temp_dir/test.pid*)
	if ! grep -q 'ERROR SUMMARY: 0 error' $log; then
		echo "Fail in $in_dict $1 checking detected by Valgrind"
		echo "$log Valgrind log file moved to $temp_dir/badlogs"
		mv $log $temp_dir/badlogs
		exit 1
	fi
	if grep -q 'LEAK SUMMARY' $log; then
		echo "Memory leak in $in_dict $1 checking detected by Valgrind"
		echo "$log Valgrind log file moved to $temp_dir/badlogs"
		mv $log $temp_dir/badlogs
		exit 1
	fi
	rm -f $log
fi
}

function test-good {
# Tests good words
in_file="$in_dict.good"

if [[ -f $in_file ]]; then
	out=$(nuspell -l -i utf8 -d "$in_dict" < "$in_file" \
	      | tr -d $'\r')
	if [[ $? -ne 0 ]]; then exit 2; fi
	if [[ "$out" != "" ]]; then
		echo "============================================="
		echo "Fail in $in_dict.good. Good words recognised as wrong:"
		echo "$out"
		exit 1
	fi
fi
check_valgrind_log "good words"
}

function test-bad {
# Tests bad words
in_file="$in_dict.wrong"

if [[ -f $in_file ]]; then
	out=$(nuspell -G -i utf8 -d "$in_dict" < "$in_file" \
	      | tr -d $'\r') #strip carige return for mingw builds
	if [[ $? -ne 0 ]]; then exit 2; fi
	if [[ "$out" != "" ]]; then
		echo "============================================="
		echo "Fail in $in_dict.wrong. Bad words recognised as good:"
		echo "$out"
		exit 1
	fi
fi
check_valgrind_log "bad words"
}

function test-morph {
# Tests morphological analysis
in_file="$in_dict.good"
expected_file="$in_dict.morph"

#in=$(sed 's/	$//' "$in_file") #passes without this.
out=$(analyze "$in_dict.aff" "$in_dict.dic" "$in_file" \
      | tr -d $'\r') #strip carige return for mingw builds
if [[ $? -ne 0 ]]; then exit 2; fi
expected=$(<"$expected_file")
if [[ "$out" != "$expected" ]]; then
	echo "============================================="
	echo "Fail in $in_dict.morph. Bad analysis?"
	diff "$expected_file" <(echo "$out") | grep '^<' | sed 's/^..//'
	exit 1
fi
check_valgrind_log "morphological analysis"
}

function test-sug {
# Tests suggestions
in_file=$in_dict.wrong
expected_file=$in_dict.sug

out=$(nuspell -i utf8 -a -d "$in_dict" <"$in_file" | \
      { grep -a '^&' || true; } | sed 's/^[^:]*: //')
if [[ $? -ne 0 ]]; then exit 2; fi
expected=$(<"$expected_file")
if [[ "$out" != "$expected" ]]; then
	echo "============================================="
	echo "Fail in $in_dict.sug. Bad suggestion?"
	diff "$expected_file" <(echo "$out")
	exit 1
fi
check_valgrind_log "suggestion"
}

# script starts here

# set -x # uncomment for debugging
set -o pipefail
export LC_ALL="C"
temp_dir=./testSubDir
test_name="$1"
[[ "$NUSPELL" = "" ]] && NUSPELL="$(dirname $0)"/../src/nuspell/nuspell
[[ "$ANALYZE" = "" ]] && ANALYZE="$(dirname $0)"/../src/tools/analyze
shopt -s expand_aliases
alias nuspell='"$NUSPELL"'
alias analyze='"$ANALYZE"'

if [[ "$VALGRIND" != "" ]]; then
	mkdir $temp_dir 2> /dev/null || :
	rm -f $temp_dir/test.pid* || :
	mkdir $temp_dir/badlogs 2> /dev/null || :
	alias nuspell='valgrind --tool=$VALGRIND --leak-check=yes --show-reachable=yes --log-file=$temp_dir/test.pid "$NUSPELL"'
	alias analyze='valgrind --tool=$VALGRIND --leak-check=yes --show-reachable=yes --log-file=$temp_dir/test.pid "$ANALYZE"'
fi

if [[ ! -f "$test_name" ]]; then
	echo "Test file $test_name does not exist."
	exit 3
fi

case $test_name in
*.dic)
	in_dict=${test_name%.dic}
	test-good
	test-bad
	;;
*.sug)
	in_dict=${test_name%.sug}
	test-sug
	;;
*.morph)
	in_dict=${test_name%.morph}
	test-morph
	;;
*)
	echo "Unsupported test type, only .sug and .dic are supported"
	exit 3
	;;
esac
