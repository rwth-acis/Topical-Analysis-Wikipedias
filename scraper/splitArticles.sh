#!/bin/bash
head -n 4664857 ../../media/english/index-articles.txt > ../../media/english/index-articles\(1\).txt
head -n 9329713 ../../media/english/index-articles.txt | tail -n 4664857 > ../../media/english/index-articles\(2\).txt
cat ../../media/english/index-articles.txt | tail -n 4664856 > ../../media/english/index-articles\(3\).txt
