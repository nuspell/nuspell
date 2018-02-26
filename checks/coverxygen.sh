# Reporting Doxygen documentation coverage
# See https://github.com/psycofdj/coverxygen

#sudo pip3 install coverxygen
#sudo apt-get install lcov

rm -rf coverxygen
mkdir coverxygen
python3 -m coverxygen --src-dir ../src/nuspell --xml-dir ../doxygen/xml --output coverxygen/doc-coverage.info
sed -i -e 's/src\/nuspell\/src\/nuspell/src\/nuspell/g' coverxygen/doc-coverage.info
lcov --summary coverxygen/doc-coverage.info
genhtml --title 'Documentation Coverage' --legend --no-function-coverage --no-branch-coverage coverxygen/doc-coverage.info -o coverxygen
