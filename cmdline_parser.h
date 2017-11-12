#pragma once
#include "common.h"
#include "scene.h"

struct MRT_Params {
    uint32 windowWidth = 500;
    uint32 windowHeight = 500;
    uint32 bufferWidth = windowWidth;
    uint32 bufferHeight = windowHeight;
    uint32 samplesPerPixel = 256;
    uint32 tileSize = 32;
    uint32 numThreads = 0; // 0 == automatic
    uint32 maxBounces = 32;
    uint32 sceneSelect = SCENE_TRIANGLES;
    uint32 threadingMode = 1; // use mode=0 and threads=1 for a deterministic runtime test
    float  maxLuminance = 1000; // luminance values can be clamped for faster convergence, but low values lead to bias
    bool   delay = false; // delayed start for recording
};

void ParseArgv(int argc, char** argv, MRT_Params *p_out);
void PrintCmdLineHelp();

template<typename T> 
int ReadParameter(int argc, char *argv[], const char *parameter, T *res_p, T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max());
int CheckParameter(int argc, char *argv[], const char *parameter);