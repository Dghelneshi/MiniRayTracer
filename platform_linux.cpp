#include "platform.h"
#include "main.h"
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/syscall.h>
#include <sys/resource.h>
#include <string.h>
#include <SDL.h>

using namespace MRT;

static uint32_t G_windowWidth;
static uint32_t G_windowHeight;
static uint32_t G_bufferWidth;
static uint32_t G_bufferHeight;

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;

uint64_t MRT_GetTime() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t) ts.tv_nsec + (uint64_t) ts.tv_sec * 1000000000ull;
}

float MRT_TimeDelta(uint64_t start, uint64_t stop) { // returns seconds
    return ((stop - start) / 1000000000.0);
}

void MRT_PlatformInit() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0){
        MRT_DebugPrint(SDL_GetError());
        exit(1);
    }
}

void MRT_SetWindowTitle(const char *str) {
    SDL_SetWindowTitle(window, str);
}

void MRT_CreateWindow(uint32_t windowWidth, uint32_t windowHeight, uint32_t bufferWidth, uint32_t bufferHeight) {
 
    window = SDL_CreateWindow("Hello World!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, 0);
    MRT_Assert(window, SDL_GetError());

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    MRT_Assert(renderer, SDL_GetError());

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, bufferWidth, bufferHeight);

    G_windowWidth  = windowWidth;
    G_windowHeight = windowHeight;
    G_bufferWidth  = bufferWidth;
    G_bufferHeight = bufferHeight;
}

void MRT_HandleMessages() {
    SDL_Event e;
    while (SDL_PollEvent(&e)){
        if (e.type == SDL_QUIT){
            WindowCallback(MRT_CLOSE);
        }
        if (e.type == SDL_KEYDOWN){
            KeyboardCallback(0, MRT_DOWN, MRT_NONE);
        }
        if (e.type == SDL_MOUSEBUTTONDOWN){
            MouseCallback(0, 0, MRT_DOWN, MRT_NONE);
        }
    }
}

void MRT_DrawToWindow(const uint32_t* backBuffer) {
    int pitch;
    void* pixels;
    SDL_LockTexture(texture, nullptr, &pixels, &pitch);
    memcpy(pixels, backBuffer, G_bufferHeight * pitch);
    SDL_UnlockTexture(texture);

    SDL_RenderClear(renderer);
    SDL_RenderCopyEx(renderer, texture, NULL, NULL, 0, NULL, SDL_FLIP_VERTICAL);
    SDL_RenderPresent(renderer);
}

void MRT_DebugPrint(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}


void MRT_Assert(bool cond) {
    assert(cond);
}

void MRT_Assert(bool cond, const char *msg) {
    if (!cond) MRT_DebugPrint(msg);
    MRT_Assert(cond);
}

void MRT_LowerThreadPriority() {
    // NOTE: POSIX states that setpriority/niceness affects the whole process, but Linux implements it per thread.
    pid_t tid = syscall(SYS_gettid);
    setpriority(PRIO_PROCESS, tid, 20);
}

void MRT_PlatformDestroy() {
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void MRT_Sleep(uint32_t ms) {
    usleep(ms * 1000u);
}
