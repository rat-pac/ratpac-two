#!/bin/bash

echo "Checking formatting..."
changes=0
for file in `find ./src \( -name "*.cc" -o -name "*.hh" -o -name "*.icc" -o -name "*.cpp" \)`;
do
    retval=`clang-format -style=file -n -Werror $file`
    if [ $? -eq 1 ]; then
        echo "Formatting $file"
        clang-format -style=file -i $file
        changes=1
    fi
done

if [ $changes -eq 0 ]; then
    exit 0
else
    echo "Formatting changes made, please commit them"
    exit 1
fi
