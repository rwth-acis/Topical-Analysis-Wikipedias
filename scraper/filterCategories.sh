#!/bin/bash

grep [0-9]*:[0-9]*:Category: ../../media/english/rawFiles/enwiki-20180701-pages-articles-multistream-index.txt > ../../media/english/index-categories.txt
echo "Filtered Categories"
sed -i 's/:Category:/:/g' ../../media/english/index-categories.txt
echo "Removed Category: prefix"
sed -i 's/[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]://g' ../../media/english/index-categories.txt
echo "Removed 11 integer long line indeces"
sed -i 's/[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]://g' ../../media/english/index-categories.txt
echo "Removed 10 integer long line indeces"
sed -i 's/$/|/' ../../media/english/index-categories.txt
echo "done"
# Adding the | character at the end of each line is not necessary as useing \n as end-of-line delimiter would work as well
