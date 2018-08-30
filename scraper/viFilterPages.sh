#!/bin/bash
# filter namespaces (only articles and categories remain)
sed '/[0-9]*:[0-9]*:Tập tin:*/d' ../../media/vietnamese/rawFiles/viwiki-20180701-pages-articles-multistream-index.txt |
sed '/[0-9]*:[0-9]*:Bản mẫu:*/d' |
#sed '/[0-9]*:[0-9]*:Draft:*/d' | does not exist in viwiki
sed '/[0-9]*:[0-9]*:Trợ giúp:*/d' |
sed '/[0-9]*:[0-9]*:Wikipedia:*/d' |
sed '/[0-9]*:[0-9]*:Chủ đề:*/d' |
sed '/[0-9]*:[0-9]*:MediaWiki:*/d' |
sed '/[0-9]*:[0-9]*:Mô đun:*/d' |
#sed '/[0-9]*:[0-9]*:Book:*/d' | does not exist in viwiki
# reverse escape
sed 's/\&amp;/\&/g' | sed 's/\&quot;/\"/g' |
# remove line indicator and write to file
sed 's/[0-9]*://' > ../../media/vietnamese/index-pages.txt
echo "done"
