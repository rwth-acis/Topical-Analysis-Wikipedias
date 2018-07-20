#!/bin/bash
for i in 1 2 3 4 5 6
do
#        echo | arangoimp --file "../../media/english/jsonFiles/articles($i).json" --type json --collection "enArticles" --batch-size 200000000
    echo | arangoimp --file "../../media/english/jsonFiles/categoryLinks($i).json" --type json --collection "enCategoryLinks" --batch-size 200000000
done
#echo | arangoimp --file "../../media/english/jsonFiles/categories.json" --type json --collection "enCategories" --batch-size 200000000
