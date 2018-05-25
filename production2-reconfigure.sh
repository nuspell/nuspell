autoreconf -vfi
if [ $? -eq 0 ]; then
	./configure --with-warnings CXXFLAGS='-g -O2 -fno-omit-frame-pointer'
else
	echo FAILED
	exit $?
fi
