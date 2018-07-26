#pragma once

#include "common.h"
#include "mrt_math.h"
#include <atomic>
#include <algorithm>

struct tile {
    uint32 xMin;
    uint32 xMax;
    uint32 yMin;
    uint32 yMax;
};

class work_queue {
public:
    tile *worklist;
    uint64 numTiles;
    uint32 numThreads;
    std::atomic<uint64> counter;

    work_queue(uint32 bufferWidth, uint32 bufferHeight, uint32 tileSize, uint32 numThreads);

    virtual tile* getWork(uint32* curSample_out) = 0;
    virtual float getPercentDone() = 0;
    virtual ~work_queue() {
        free(worklist);
    }
};

class work_queue_seq final : public work_queue {
public:
    work_queue_seq(uint32 bufferWidth, uint32 bufferHeight, uint32 tileSize, uint32 numThreads)
                  : work_queue(bufferWidth, bufferHeight, tileSize, numThreads) {}

    tile* getWork(uint32* curSample_out);
    float getPercentDone();
};

class work_queue_dynamic final : public work_queue {
public:
    uint32 numSamples;

    work_queue_dynamic(uint32 bufferWidth, uint32 bufferHeight, uint32 tileSize, uint32 numThreads, uint32 numSamples)
                      : work_queue(bufferWidth, bufferHeight, tileSize, numThreads),
                        numSamples(numSamples) {}

    tile* getWork(uint32* curSample_out);
    float getPercentDone();
};