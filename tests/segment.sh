set -ex
../src/nuspell/nuspell -d v1cmdline/base segment.txt 2> /dev/null
../src/nuspell/nuspell -S -d v1cmdline/base segment.txt 2> /dev/null
../src/nuspell/nuspell -U -d v1cmdline/base segment.txt 2> /dev/null
../src/nuspell/nuspell -S -U -d v1cmdline/base segment.txt 2> /dev/null
../src/nuspell/nuspell -l -d v1cmdline/base segment.txt 2> /dev/null
../src/nuspell/nuspell -G -d v1cmdline/base segment.txt 2> /dev/null
../src/nuspell/nuspell -G -L -d v1cmdline/base segment.txt 2> /dev/null
../src/nuspell/nuspell -l -L -d v1cmdline/base segment.txt 2> /dev/null
