#!/bin/bash
#for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27
#do
        echo | arangoimp --file "../media/english/jsonFiles/articles.json" --type json --collection "enArticles" --batch-size 200000000
        echo | arangoimp --file "../media/english/jsonFiles/categories.json" --type json --collection "enCategories"
        echo | arangoimp --file "../media/english/jsonFiles/categoryLinks.json" --type json --collection "enCategoryLinks"
#done

