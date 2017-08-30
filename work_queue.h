#pragma once

#include "common.h"
#include <atomic>

struct work {
    uint32 xMin;
    uint32 xMax;
    uint32 yMin;
    uint32 yMax;
};

class work_queue {
public:
    work *worklist;
    uint32 workCount;
    std::atomic<uint32> workAllocated;

    work_queue(uint32 bufferWidth, uint32 bufferHeight, uint32 packetSize, uint32 *totalWork) {

        workAllocated = 0;

        uint32 xCount = (bufferWidth  + (packetSize - 1u)) / packetSize;
        uint32 yCount = (bufferHeight + (packetSize - 1u)) / packetSize;

        workCount = (xCount * yCount);
        *totalWork = workCount;

        worklist = (work*) malloc(sizeof(*worklist) * workCount);

        for (uint32 y = 0; y < yCount; y++)
        {
            for (uint32 x = 0; x < xCount; x++)
            {
                // last packet in each row and column can be smaller than packetSize

                uint32 xMin = x * packetSize;
                uint32 xMax = std::min(xMin + packetSize, bufferWidth);
                uint32 yMin = y * packetSize;
                uint32 yMax = std::min(yMin + packetSize, bufferHeight);

                worklist[x + y * xCount] = { xMin, xMax, yMin, yMax };
            }
        }
    }

    work* getWork() {
        uint32 cur = workAllocated++;
        
        if (cur >= workCount)
            return nullptr;

        return &worklist[cur];
    }

    ~work_queue() {
        free(worklist);
    }

};


// TODO: try implementing a single synchronized worklist like above to prevent threads from lagging behind too much
// possibly: curWork[id] can be > workCount, calc curLoop[id] by div or looped subtraction

// each thread gets separate work, can loop through each work list multiple times
class work_queue_multi {
public:
    work **worklist;
    uint32 *workCount;
    uint32 *curWork;
    uint32 *curLoop;
    uint32 numThreads;
    uint32 maxLoops;

    work_queue_multi(uint32 bufferWidth, uint32 bufferHeight, uint32 packetSize, uint32 numThreads, uint32 maxLoops, uint32 *totalWork) : numThreads(numThreads), maxLoops(maxLoops) {

        worklist  = (work**) calloc(numThreads, sizeof(*worklist));
        workCount = (uint32*) calloc(numThreads, sizeof(*workCount));
        curWork   = (uint32*) calloc(numThreads, sizeof(*curWork));
        curLoop   = (uint32*) calloc(numThreads, sizeof(*curLoop));

        uint32 xCount = (bufferWidth  + (packetSize - 1u)) / packetSize;
        uint32 yCount = (bufferHeight + (packetSize - 1u)) / packetSize;

        uint32 maxWorkCount = yCount * ((xCount / numThreads) + 1u); // maximum possible work amount per thread, for ease of allocation
        for (uint32 threadId = 0; threadId < numThreads; threadId++) {
            worklist[threadId] = (work*) calloc(maxWorkCount, sizeof(**worklist));
        }

        uint32 threadId = 0;
        for (uint32 y = 0; y < yCount; y++)
        {
            for (uint32 x = 0; x < xCount; x++)
            {
                uint32 xMin = x * packetSize;
                uint32 xMax = std::min(xMin + packetSize, bufferWidth);
                uint32 yMin = y * packetSize;
                uint32 yMax = std::min(yMin + packetSize, bufferHeight);

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
    work* getWork(uint32 threadId, uint32* curLoop_p) const {
        uint32 cur = curWork[threadId];
        
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
        for (uint32 threadId = 0; threadId < numThreads; threadId++) {
            free(worklist[threadId]);
        }
        free(worklist);
        free(workCount);
        free(curWork);
    }

};