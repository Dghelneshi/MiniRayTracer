#pragma once
#include "common.h"

void MRT_PlatformInit();
void MRT_PlatformDestroy();
void MRT_HandleMessages();
void MRT_CreateWindow(uint32_t windowWidth, uint32_t windowHeight, uint32_t bufferWidth, uint32_t bufferHeight);
void MRT_SetWindowTitle(const char *str);
void MRT_DrawToWindow(const uint32_t *backBuffer);

void MRT_DebugPrint(const char *format, ...);
void MRT_Assert(bool cond);
void MRT_Assert(bool cond, const char *msg);
void MRT_Sleep(uint32_t ms);

void MRT_LowerThreadPriority();

uint64_t MRT_GetTime();
float MRT_TimeDelta(uint64_t start, uint64_t stop); // returns seconds