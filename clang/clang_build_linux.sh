# !/bin/bash

files=""
# include all cpp files, but only the correct platform layer
for f in ../*.cpp; do
    if [ `expr "$f" : '.*platform_'` -gt 0 ]; then
        fnord="${f##../}"
        if [ "${fnord##platform_}" = "linux.cpp" ]; then
            files="$files $f"
        fi
    else
        files="$files $f"
    fi
done

# more generic alternative with newer bash version?
    # regex='.*platform_(.*)'
    # if [[ $f =~ $regex ]]; then
    #     if [ "${BASH_REMATCH[1]}" = "linux.cpp" ]; then
    #         files="$files $f"
    #     fi

libs="-lpthread -lSDL2"
dirs="-I../include/ -I/usr/include/SDL2"
warns="-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wno-ignored-attributes"
opts="-m64 -g -O3 -march=native"
misc="-fno-exceptions -fno-rtti"

clang++ -std=c++11 $opts $dirs $libs $warns $misc -o MiniRayTracer $files

# optional asm output
# clang++ -S -std=c++11 -masm=intel $opts $dirs $libs $misc $files
