@echo off
echo Building tests with coverage instrumentation...

cd build_coverage
cmake .. -G "MinGW Makefiles" -DHEADLESS=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=C:/msys64/ucrt64/bin/gcc.exe -DCMAKE_C_FLAGS="-coverage -fprofile-arcs -ftest-coverage -g -O0"
if %errorlevel% neq 0 exit /b %errorlevel%

echo Compiling tests...
mingw32-make -j4
if %errorlevel% neq 0 exit /b %errorlevel%

echo Running test suites...
test_autobrake.exe
test_wipers.exe
test_speedgov.exe
test_autopark.exe
test_climate.exe
test_autobrake_eval.exe

echo Generating coverage data...
gcov -b -c ../src/*.c ../tests/*.c

echo Coverage generation complete!
cd ..