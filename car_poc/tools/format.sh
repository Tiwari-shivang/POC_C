#!/bin/bash

echo "Formatting C source files..."

find src/ inc/ cfg/ -name "*.c" -o -name "*.h" | xargs clang-format -i

echo "Formatting complete."