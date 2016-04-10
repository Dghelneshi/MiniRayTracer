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

    work_queue(int32 bufferWidth, int32 bufferHeight, int32 packetSize, int32 *totalWork) {

        int32 xCount = (bufferWidth  + (packetSize - 1)) / packetSize;
        int32 yCount = (bufferHeight + (packetSize - 1)) / packetSize;

        workCount = (xCount * yCount);
        *totalWork = workCount;

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



// each thread gets separate work, can loop through each work list multiple times
class work_queue_multi {
public:
    work **worklist;
    int32 *workCount;
    int32 *curWork;
    int32 *curLoop;
    int32 numThreads;
    int32 maxLoops;

    work_queue_multi(int32 bufferWidth, int32 bufferHeight, int32 packetSize, int32 numThreads, int32 maxLoops, int32 *totalWork) : numThreads(numThreads), maxLoops(maxLoops) {

        worklist  = (work**) calloc(numThreads, sizeof(*worklist));
        workCount = (int32*) calloc(numThreads, sizeof(*workCount));
        curWork   = (int32*) calloc(numThreads, sizeof(*curWork));
        curLoop   = (int32*) calloc(numThreads, sizeof(*curLoop));

        int32 xCount = (bufferWidth  + (packetSize - 1)) / packetSize;
        int32 yCount = (bufferHeight + (packetSize - 1)) / packetSize;

        int32 maxWorkCount = yCount * ((xCount / numThreads) + 1); // maximum possible work amount per thread, for ease of allocation
        for (int threadId = 0; threadId < numThreads; threadId++) {
            worklist[threadId] = (work*) calloc(maxWorkCount, sizeof(**worklist));
        }

        int threadId = 0;
        for (int y = 0; y < yCount; y++)
        {
            for (int x = 0; x < xCount; x++)
            {
                int32 xMin = x * packetSize;
                int32 xMax = min(xMin + packetSize, bufferWidth);
                int32 yMin = y * packetSize;
                int32 yMax = min(yMin + packetSize, bufferHeight);

                worklist[threadId][workCount[threadId]] = { xMin, xMax, yMin, yMax };

                workCount[threadId]++;
                threadId++;
                threadId %= numThreads;
                if (workCount[threadId] > maxWorkCount) DebugBreak(); // OOPS!
            }
        }
        
        if (totalWork) *totalWork = xCount * yCount * maxLoops;
    }

    // each thread can loop through its work queue multiple times
    work* getWork(int32 threadId, int32* curLoop_p) {
        int32 cur = curWork[threadId];
        
        if (cur == workCount[threadId]) {
            if (curLoop[threadId] == (maxLoops - 1))
                return nullptr;

            cur -= workCount[threadId];
            curWork[threadId] -= workCount[threadId];
            curLoop[threadId]++;
        }
        *curLoop_p = curLoop[threadId];

        curWork[threadId]++;
        return &worklist[threadId][cur % workCount[threadId]];
    }

    ~work_queue_multi() {
        for (int threadId = 0; threadId < numThreads; threadId++) {
            free(worklist[threadId]);
        }
        free(worklist);
        free(workCount);
        free(curWork);
    }

};