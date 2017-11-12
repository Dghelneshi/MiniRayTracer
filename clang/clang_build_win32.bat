@echo off
SETLOCAL

REM include all cpp files, but only the correct platform layer
for %%a in (../*.cpp) do (
    for /f "tokens=1 delims=_" %%n in ("%%~na") do (
        if "%%n"=="platform" (
            REM include only platform_win32.cpp
            for /f "tokens=2 delims=_" %%n in ("%%~na") do (
                if "%%n"=="win32" (
                    call SET files=%%files%% ../%%a
                )
            )
        ) else (
            REM include all files that are not called platform_xxx.cpp
            call SET files=%%files%% ../%%a
        )
    )
)

SET libs=-lkernel32 -luser32 -lgdi32
SET dirs=-I../include/
SET warns=-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
SET opts=-m64 -O3 -march=native
SET misc=-fno-exceptions -fno-rtti -D_CRT_SECURE_NO_WARNINGS -Xclang -flto-visibility-public-std

REM C++14 is required if compiling with the Visual Studio 2017 headers
clang++ -std=c++14 %opts% %dirs% %libs% %warns% %misc% -o MiniRayTracer.exe %files%

REM optional asm output
REM clang++ -S -std=c++14 -masm=intel %opts% %dirs% %libs% %misc% %files%

ENDLOCAL