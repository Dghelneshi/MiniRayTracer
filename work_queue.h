#pragma once

#include <Windows.h> // InterlockedIncrement (maybe use std::atomic instead?)
#include "common.h"

struct work {
    int32 xMin;
    int32 xMax;
    int32 yMin;
    int32 yMax;
};

class work_queue {
public:
    work *worklist;
    int32 workCount;
    volatile LONG workAllocated = 0;

    work_queue(int32 bufferWidth, int32 bufferHeight, int32 packetSize) {

        int32 xCount = (bufferWidth  + (packetSize - 1)) / packetSize;
        int32 yCount = (bufferHeight + (packetSize - 1)) / packetSize;

        workCount = (xCount * yCount);

        worklist = (work*) malloc(sizeof(*worklist) * workCount);

        for (int y = 0; y < yCount; y++)
        {
            for (int x = 0; x < xCount; x++)
            {
                // last packet in each row and column can be smaller than packetSize

                int32 xMin = x * packetSize;
                int32 xMax = min(xMin + packetSize, bufferWidth);
                int32 yMin = y * packetSize;
                int32 yMax = min(yMin + packetSize, bufferHeight);

                worklist[x + y * xCount] = { xMin, xMax, yMin, yMax };
            }
        }
    }

    work* getWork() {
        int32 cur = InterlockedIncrement(&workAllocated) - 1;
        
        if (cur >= workCount)
            return nullptr;

        return &worklist[cur];
    }

    ~work_queue() {
        free(worklist);
    }

};