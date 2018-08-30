#!/bin/bash
# filter namespaces (only articles and categories remain)
sed '/[0-9]*:[0-9]*:File:*/d' ../../media/english/rawFiles/enwiki-20180701-pages-articles-multistream-index.txt |
sed '/[0-9]*:[0-9]*:Template:*/d' |
sed '/[0-9]*:[0-9]*:Draft:*/d' |
sed '/[0-9]*:[0-9]*:Help:*/d' |
sed '/[0-9]*:[0-9]*:Wikipedia:*/d' |
sed '/[0-9]*:[0-9]*:Portal:*/d' |
sed '/[0-9]*:[0-9]*:MediaWiki:*/d' |
sed '/[0-9]*:[0-9]*:Module:*/d' |
sed '/[0-9]*:[0-9]*:Book:*/d' |
# reverse escape
sed 's/\&amp;/\&/g' | sed 's/\&quot;/\"/g' |
# remove line indicator and write to file
sed 's/[0-9]*://' > ../../media/english/index-pages.txt
echo "done"
