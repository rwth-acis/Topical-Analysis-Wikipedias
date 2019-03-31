#!/bin/bash
# filter namespaces (only articles and categories remain)
sed '/[0-9]*:[0-9]*:Mediji*/d' ../../media/serbo-croat/rawFiles/shwiki-20181001-pages-articles-multistream-index.txt |
sed '/[0-9]*:[0-9]*:Šablon:*/d' |
sed '/[0-9]*:[0-9]*:Datoteka:*/d' |
sed '/[0-9]*:[0-9]*:Wikipedia:*/d' |
sed '/[0-9]*:[0-9]*:Portal:*/d' |
sed '/[0-9]*:[0-9]*:MediaWiki:*/d' |
sed '/[0-9]*:[0-9]*:Pomoć:*/d' |
# reverse escape
sed 's/\&amp;/\&/g' | sed 's/\&quot;/\"/g' |
# remove line indicator and write to file
sed 's/[0-9]*://' > ../../media/serbo-croat/index-pages.txt
echo "done"
