#include <cstdio>
#include <math.h>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>

#include "common.h"
#include "scene.h"

#include "cmdline_parser.h"

static MRT_Params G_params;

// TODO: better error checking

// reads a value from a string argument
template<typename T> T Read(char *argv);

template<> float Read(char *argv) {
    return strtof(argv, nullptr);
}
template<> uint32 Read(char *argv) {
    return strtoul(argv, nullptr, 0);
}
template<> char* Read(char *argv) {
    return argv;
}

// allows us to read in strings with the same ReadParameter function
#if _MSC_VER && !__clang__
#define __builtin_constant_p(a) 0
#endif
template<>
constexpr char* std::numeric_limits<char*>::max() noexcept {
    return __builtin_constant_p((char*) UINTPTR_MAX) ? (char*) UINTPTR_MAX : (char*) UINTPTR_MAX;
}

// tries to read a parameter from the command line
template<typename T>
int ReadParameter(int argc, char *argv[], const char *parameter, T *res_p, T min, T max) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(parameter, argv[i]) == 0) {

            if ((i + 1) == argc) {
                std::cout << "Warning: Missing value for parameter '" << parameter << "'." << std::endl;
                return 0;
            }
            
            T p = Read<T>(argv[i + 1]);
            if ((p < min) || (p > max)) {
                std::cout << "Warning: Invalid value for parameter '" << parameter << "', must be in [" << min << ", " << max << "]." << std::endl;
                return 0;
            }
            else {
                *res_p = p;
                return i;
            }
        }
    }
    return 0;
}

int CheckParameter(int argc, char *argv[], const char *parameter) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(parameter, argv[i]) == 0) {
            return i;
        }
    }
    return 0;
}

// TODO: use static singleton class instead?
MRT_Params *getParams() {
    return &G_params;
}

void ParseArgv(int argc, char** argv) {

    if (CheckParameter(argc, argv, "-help")  ||
        CheckParameter(argc, argv, "--help") ||
        CheckParameter(argc, argv, "-?")) {

        PrintCmdLineHelp();
        exit(0);
    }

    MRT_Params p;

    if (ReadParameter(argc, argv, "-width" , &p.windowWidth , 1u))
        p.bufferWidth  = p.windowWidth;
    if (ReadParameter(argc, argv, "-height", &p.windowHeight, 1u))
        p.bufferHeight = p.windowHeight;

    ReadParameter(argc, argv, "-samples",  &p.samplesPerPixel, 1u);
    ReadParameter(argc, argv, "-tilesize", &p.tileSize, 1u);
    ReadParameter(argc, argv, "-threads",  &p.numThreads);
    ReadParameter(argc, argv, "-depth",    &p.maxBounces);
    ReadParameter(argc, argv, "-scene",    &p.sceneSelect, 0u, ENUM_SCENES_MAX - 1u);
    ReadParameter(argc, argv, "-mode",     &p.threadingMode, 0u, 1u);
    ReadParameter(argc, argv, "-maxlum",   &p.maxLuminance);

    p.delay = CheckParameter(argc, argv, "-delay");

    G_params = p;
}


void PrintCmdLineHelp() {
    printf("\n" \
           "PARAMETERS:\n" \
           "  -width    \t<value>\t\tWindow width\n" \
           "  -height   \t<value>\t\tWindow height\n" \
           "  -samples  \t<value>\t\tSamples per pixel\n" \
           "  -depth    \t<value>\t\tMaximum bounce depth per primary ray\n" \
           "  -maxlum   \t<value>\t\tClamp maximum luminance (introduces bias)\n" \
           "  -threads  \t<value>\t\tNumber of execution threads (0 selects maximum hardware threads)\n" \
           "  -tilesize \t<value>\t\tSize of image tiles (threads operate on tiles)\n" \
           "  -mode     \t[0, 1]\t\tThreading/queue mode (0 for sequential, 1 for dynamic sampling)\n" \
           "  -scene    \t[0, %i]\t\tSelect the scene\n" \
           "  -delay    \t\t\tDelay start until keypress\n", ENUM_SCENES_MAX - 1);
    // TODO: find a commonly understood term for the threading modes
}