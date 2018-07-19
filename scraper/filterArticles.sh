#!/bin/bash
sed '/[0-9]*:[0-9]*:Category*/d' ../../media/english/rawFiles/enwiki-20180701-pages-articles-multistream-index.txt > ../../media/english/index-articles.txt
sed -i '/[0-9]*:[0-9]*:File*/d' ../../media/english/index-articles.txt
sed -i '/[0-9]*:[0-9]*:Template*/d' ../../media/english/index-articles.txt
sed -i '/[0-9]*:[0-9]*:Draft*/d' ../../media/english/index-articles.txt
sed -i '/[0-9]*:[0-9]*:Help*/d' ../../media/english/index-articles.txt
sed -i '/[0-9]*:[0-9]*:Wikipedia*/d' ../../media/english/index-articles.txt
