cd ..

cloc \
--3 --by-file src/nuspell \
> checks/cloc.txt

#cloc \
#--3 --by-file src/nuspell \
#--xml > checks/cloc.xml

cd checks
