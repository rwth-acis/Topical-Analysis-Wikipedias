#!/bin/bash
# filter namespaces (only articles and categories remain)
sed '/[0-9]*:[0-9]*:ויקיפדיה:*/d' ../../media/hebrew/rawFiles/hewiki-20180801-pages-articles-multistream-index.txt |
sed '/[0-9]*:[0-9]*:קובץ:*/d' |
sed '/[0-9]*:[0-9]*:מדיה ויקי:*/d' |
sed '/[0-9]*:[0-9]*:תבנית:*/d' |
sed '/[0-9]*:[0-9]*:עזרה:*/d' |
sed '/[0-9]*:[0-9]*:טיוטה:*/d' |
sed '/[0-9]*:[0-9]*:יחידה:*/d' |
sed '/[0-9]*:[0-9]*:נושא:*/d' |
sed '/[0-9]*:[0-9]*:פורטל:*/d' |
# reverse escape
sed 's/\&amp;/\&/g' | sed 's/\&quot;/\"/g' |
# remove line indicator and write to file
sed 's/[0-9]*://' > ../../media/hebrew/index-pages.txt
echo "done"
