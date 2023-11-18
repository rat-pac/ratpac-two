#!/bin/bash

if command -v clang-format &>/dev/null; then
    echo "clang-format is installed."
else
    echo "clang-format is not installed. Formatting rules won't be auto applied upon commit."
    echo "Consider installing clang-format to ensure consistent formatting across the project, especially if you \
plan to contribute."
    exit 1
fi

echo "Checking formatting..."
changes=0
for file in $(find ./src \( -name "*.cc" -o -name "*.hh" -o -name "*.icc" -o -name "*.cpp" -o -name "*.hpp" \)); do
    retval=$(clang-format -style=file -n -Werror $file)
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
