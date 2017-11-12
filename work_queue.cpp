#include "work_queue.h"


tile* work_queue_seq::getWork(uint32* curSample_out) {
    uint64 cur = counter.fetch_add(1, std::memory_order_relaxed);

    if (cur >= numTiles)
        return nullptr;

    return &worklist[cur];
}

float work_queue_seq::getPercentDone() {
    // the counter is incremented at the *start* of work, so we need to subtract one count per thread
    int64 c = int64(counter) - int64(numThreads);
    if (c >= int64(numTiles))
        return 100.0f; // ensure this is exact
    else
        return (c * 100) / float(numTiles);
}


// TODO: Possible race condition since one thread can round trip faster than the other and work on the same tile.
//       Unlikely and small effect with small tiles, but worth considering. Keep track of which index each thread is on and skip that one for later somehow?
//       Could increment a "repeat tile" counter for each thread if another thread encounters the same tile, but that will *probably* not eliminate 
//       all possible race conditions, just make them extremely unlikely.

tile* work_queue_dynamic::getWork(uint32* curSample_out) {
    uint64 cur = counter.fetch_add(1, std::memory_order_relaxed); // get current counter, advance
    if (cur >= (numTiles * numSamples)) // no more work to be done
        return nullptr;

    uint64 cur_work = cur % numTiles; // get work index for counter
    *curSample_out = uint32(cur / numTiles); // return current sample index
    return &worklist[cur_work];
}

float work_queue_dynamic::getPercentDone() {
    // the counter is incremented at the *start* of work, so we need to subtract one count per thread
    int64 c = int64(counter) - int64(numThreads);
    if (c >= int64(numTiles * numSamples))
        return 100.0f; // ensure this is exact
    else
        return (c * 100) / float(numTiles * numSamples);
}