echo blocking bug https://github.com/jessevdk/cldoc/issues/130
exit 1

if [ -z `which cldoc` ]; then
	sudo pip3 install cldoc
fi
exit
rm -rf cld
mkdir cld
cldoc generate --output cld ../src/nuspell/*.?xx
