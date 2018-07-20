#!/bin/bash
sed '/[0-9]*:[0-9]*:Category:*/d' ../../media/english/rawFiles/enwiki-20180701-pages-articles-multistream-index.txt > ../../media/english/index-articles.txt
echo "Filtered out category namespace"
sed -i '/[0-9]*:[0-9]*:File:*/d' ../../media/english/index-articles.txt
echo "Filtered out file namespace"
sed -i '/[0-9]*:[0-9]*:Template:*/d' ../../media/english/index-articles.txt
echo "Filtered out template namespace"
sed -i '/[0-9]*:[0-9]*:Draft:*/d' ../../media/english/index-articles.txt
echo "Filtered out draft namespace"
sed -i '/[0-9]*:[0-9]*:Help:*/d' ../../media/english/index-articles.txt
echo "Filtered out help namespace"
sed -i '/[0-9]*:[0-9]*:Wikipedia:*/d' ../../media/english/index-articles.txt
echo "Filtered out Wikipedia namespace"
sed -i '/[0-9]*:[0-9]*:Portal:*/d' ../../media/english/index-articles.txt
echo "Filtered out portal namespace"
sed -i '/[0-9]*:[0-9]*:MediaWiki:*/d' ../../media/english/index-articles.txt
echo "Filtered out MediaWiki namespace"
sed -i '/[0-9]*:[0-9]*:Module:*/d' ../../media/english/index-articles.txt
echo "Filtered out module namespace"
sed -i '/[0-9]*:[0-9]*:Book:*/d' ../../media/english/index-articles.txt
echo "Filtered out book namespace"
sed -i 's/$/|/' ../../media/english/index-articles.txt
echo "done"
# Adding the | character at the end of each line is not necessary as useing \n as end-of-line delimiter would work as well
