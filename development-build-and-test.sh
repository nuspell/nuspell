cd src/nuspell
./clang-format.sh
cd ../../tests
./clang-format.sh
cd ..
make
if [ $? -eq 0 ]; then
	make check
else
	echo FAILED
	exit $?
fi
