#!/bin/bash

host=https://www.unicode.org/Public/11.0.0/ucd

rm -rf *.txt

wget ${host}/CaseFolding.txt -O CaseFolding.txt
wget ${host}/extracted/DerivedGeneralCategory.txt -O GeneralCategory.txt
wget ${host}/UnicodeData.txt -O UnicodeData.txt
wget ${host}/Scripts.txt -O Scripts.txt # for RE2

touch *.txt

tar czf unicode-data.tgz *.txt

rm -rf *.txt
