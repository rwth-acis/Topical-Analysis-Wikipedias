#!/bin/bash

grep [0-9]*:[0-9]*:Category: ../../media/english/rawFiles/enwiki-20180701-pages-articles-multistream-index.txt > ../../media/english/hashmap
sed -i 's/:Category:/:/g' ../../media/english/hashmap
sed -i 's/[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]://g' ../../media/english/hashmap
sed -i 's/[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]://g' ../../media/english/hashmap
