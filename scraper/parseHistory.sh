#!/bin/bash
for i in {1..27}
do
    echo "Decompressing history file $i"
    ark -be ../../media/english/compressedFiles/history/enwiki-20180701-pages-meta-history$i.xml.7z & wait $!
    echo "Parsing history file $i"
    echo $i | ./scraper -h
    echo "Deleting decompressed files"
    rm ../../media/english/compressedFiles/history/enwiki-20180701-pages-meta-history$i.xml
done
