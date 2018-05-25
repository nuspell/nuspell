make CXXFLAGS='-g -O2 -fno-omit-frame-pointer'
if [ $? -ne 0 ]; then
	echo FAILED
	exit $?
fi
