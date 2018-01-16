#!/usr/bin/env bash

if [ `dpkg -l gettext|grep ^un|wc -l` -eq 0 ]; then
	echo 'This script will install and deinstall gettext.'
	echo 'The package gettext is installed at the moment.'
	echo 'Please deinstall gettext with `dpkg -P gettext`.'
	echo 'If needed, install gettext aftwards again with e.g. `sudo apt-get install gettext`.'
	exit 1
fi

sudo apt-get install gettext

if [ /usr/share/aclocal/lib-link.m4 -nt m4/lib-link.m4 ]; then
	cp -f /usr/share/aclocal/lib-link.m4 m4/lib-link.m4
fi
if [ /usr/share/aclocal/lib-prefix.m4 -nt m4/lib-prefix.m4 ]; then
	cp -f /usr/share/aclocal/lib-prefix.m4 m4/lib-prefix.m4
fi
if [ /usr/share/aclocal/lib-ld.m4 -nt m4/lib-ld.m4 ]; then
	cp -f /usr/share/aclocal/lib-ld.m4 m4/lib-ld.m4
fi

sudo dpkg -P gettext
