cd src/nuspell
./clang-format.sh
cd ../../tests
./clang-format.sh
cd ..
make
make check
