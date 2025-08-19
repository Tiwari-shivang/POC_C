#!/bin/bash

echo "Running static analysis..."

echo "=== Cppcheck ==="
cppcheck --enable=warning,performance,portability --std=c99 --inline-suppr \
         --suppress=missingIncludeSystem \
         --error-exitcode=1 \
         src/ inc/ 2>&1

CPPCHECK_STATUS=$?

echo "=== clang-tidy ==="
find src/ -name "*.c" -exec clang-tidy {} -- -std=c99 -Iinc -Icfg \; 2>&1

CLANG_TIDY_STATUS=$?

echo "=== clang-format check ==="
find src/ inc/ -name "*.c" -o -name "*.h" | xargs clang-format --dry-run --Werror 2>&1

CLANG_FORMAT_STATUS=$?

echo "=== Analysis Results ==="
if [ $CPPCHECK_STATUS -eq 0 ]; then
    echo "Cppcheck: PASS"
else
    echo "Cppcheck: FAIL"
fi

if [ $CLANG_TIDY_STATUS -eq 0 ]; then
    echo "clang-tidy: PASS"
else
    echo "clang-tidy: FAIL"
fi

if [ $CLANG_FORMAT_STATUS -eq 0 ]; then
    echo "clang-format: PASS"
else
    echo "clang-format: FAIL"
fi

if [ $CPPCHECK_STATUS -ne 0 ] || [ $CLANG_TIDY_STATUS -ne 0 ] || [ $CLANG_FORMAT_STATUS -ne 0 ]; then
    echo "Static analysis FAILED"
    exit 1
else
    echo "Static analysis PASSED"
    exit 0
fi