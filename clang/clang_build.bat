@echo off
REM clang++ -S -O3 -std=c++11 -m64 -march=native -masm=intel -I../include/ ..\\main.cpp
clang++ -O3 -std=c++11 -m64 -march=native -I../include/ -mwindows -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -o MiniRayTracer.exe ../main.cpp