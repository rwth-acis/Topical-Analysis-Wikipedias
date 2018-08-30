#!/bin/bash
for filePath in "$@"
do
    basePath=$(echo $filePath | sed 's/\..*$//')
    echo "formating file $basePath"
    sed "s/\\\'/\'/g" $filePath |
    sed 's/\\/\\\\/g' |
    sed 's/\"/\\"/g' |
    sed 's/{\\"/{\"/g' |
    sed 's/\\":\\"/\":\"/g' |
    sed 's/\\",\\"/\",\"/g' |
    sed  's/\\"}/\"}/g' > $basePath\_escaped.json
done
