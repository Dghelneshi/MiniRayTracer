# !/bin/bash

files=""
# include all files from main project except main.cpp, include only the correct platform layer
for f in ../*.cpp; do
    if [ `expr "$f" : '.*platform_'` -gt 0 ]; then
        fnord="${f##../}"
        # include only platform_linux.cpp
        if [ "${fnord##platform_}" = "linux.cpp" ]; then
            files="$files $f"
        fi
    else
        # include all other files except main.cpp
        if [ "$f" != "main.cpp" ]; then
            files="$files $f"
        fi
    fi
done

# more generic alternative with newer bash version?
    # regex='.*platform_(.*)'
    # if [[ $f =~ $regex ]]; then
    #     if [ "${BASH_REMATCH[1]}" = "linux.cpp" ]; then
    #         files="$files $f"
    #     fi

libs="-lpthread -lbenchmark -lSDL2"
dirs="-Llib -Iinclude -I../include/ -I../ -I/usr/include/SDL2"
warns="-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wno-ignored-attributes"
opts="-m64 -g -O3 -march=native"
misc="-fno-exceptions -fno-rtti"

clang++ -std=c++11 $opts $dirs $libs $warns $misc -o bench $files

# optional asm output
# clang++ -S -std=c++11 -masm=intel $opts $dirs $libs $misc $files
