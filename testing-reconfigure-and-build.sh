autoreconf -vfi
if [ $? -eq 0 ]; then
	./configure --with-warnings CXXFLAGS='-O2 -fno-omit-frame-pointer'
else
	echo FAILED
	exit $?
fi
make CXXFLAGS='-O2 -fno-omit-frame-pointer'
if [ $? -ne 0 ]; then
	echo FAILED
	exit $?
fi
