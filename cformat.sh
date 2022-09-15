#!/bin/bash

find . \( -name "*.cc" -o -name "*.hh" -o -name "*.icc" -o -name "*.cpp" \) -exec clang-format --style=file -i {} \;
