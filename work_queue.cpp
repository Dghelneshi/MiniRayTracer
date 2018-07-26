#include "work_queue.h"


// https://en.wikipedia.org/wiki/Hilbert_curve

static void rot(uint32 n, uint32 *x, uint32 *y, uint32 rx, uint32 ry) {
    if (ry == 0) {
        if (rx == 1) {
            *x = n - 1 - *x;
            *y = n - 1 - *y;
        }

        uint32 t = *x;
        *x = *y;
        *y = t;
    }
}

static void hilbertDtoXY(uint32 n, uint32 d, uint32 *x, uint32 *y) {
    uint32 rx, ry, s, t = d;
    *x = *y = 0;
    for (s = 1; s < n; s *= 2) {
        rx = 1 & (t / 2);
        ry = 1 & (t ^ rx);
        rot(s, x, y, rx, ry);
        *x += s * rx;
        *y += s * ry;
        t /= 4;
    }
}

// http://graphics.stanford.edu/~seander/bithacks.html#ReverseParallel
static uint32 reverseU32(uint32 v) {
    // swap odd and even bits
    v = ((v >> 1) & 0x55555555u) | ((v & 0x55555555u) << 1);
    // swap consecutive pairs
    v = ((v >> 2) & 0x33333333u) | ((v & 0x33333333u) << 2);
    // swap nibbles ... 
    v = ((v >> 4) & 0x0F0F0F0Fu) | ((v & 0x0F0F0F0Fu) << 4);
    // swap bytes
    v = ((v >> 8) & 0x00FF00FFu) | ((v & 0x00FF00FFu) << 8);
    // swap 2-byte long pairs
    v = (v >> 16) | (v << 16);
    return v;
}

// https://stackoverflow.com/questions/4909263/how-to-efficiently-de-interleave-bits-inverse-morton
uint32 extractEven(uint32 x) {
    x = x & 0x55555555;
    x = (x | (x >> 1)) & 0x33333333;
    x = (x | (x >> 2)) & 0x0F0F0F0F;
    x = (x | (x >> 4)) & 0x00FF00FF;
    x = (x | (x >> 8)) & 0x0000FFFF;
    return x;
}

static void deInterleave(uint32 d, uint32 *x, uint32 *y) {
    *x = extractEven(d);
    *y = extractEven(d >> 1);
}

///

work_queue::work_queue(uint32 bufferWidth, uint32 bufferHeight, uint32 tileSize, uint32 numThreads) : numThreads(numThreads), counter(0) {
    uint32 xCount = (bufferWidth + (tileSize - 1u)) / tileSize;
    uint32 yCount = (bufferHeight + (tileSize - 1u)) / tileSize;
    numTiles = (xCount * yCount);

    // regular row first processing
    worklist = (tile*) malloc(sizeof(*worklist) * numTiles);

    for (uint32 y = 0; y < yCount; y++) {
        for (uint32 x = 0; x < xCount; x++) {
            // last tile in each row and column can be smaller than tileSize
            uint32 xMin = x * tileSize;
            uint32 xMax = std::min(xMin + tileSize, bufferWidth);
            uint32 yMin = y * tileSize;
            uint32 yMax = std::min(yMin + tileSize, bufferHeight);

            worklist[x + y * xCount] = { xMin, xMax, yMin, yMax };
        }
    }

#if 1
    // shuffle tiles in order of a space filling curve

#define HILBERT 1 // use Hilbert or Morton/Z-curve?
#define INVERT  1 // invert the function to result in the *least* spacially coherent curve instead

    tile* worklistFinal = (tile*) malloc(sizeof(*worklistFinal) * numTiles);
    
    // Since our tile count is not a po2 AND the picture is not square we need to apply some tricks to make Hilbert work:
    // Calculate Hilbert indices even outside of the image by rounding up max(xCount,yCount) to the nearest po2.
    // Ignore tile x/y indices outside of the image, but *do not advance the index for our list*, i.e. we skip over the part of the curve that is outside.
    // The resulting curve may no longer be connected on the edges of the image in every case, but in practice it works out quite well due to the behavior of Hilbert curves.

    uint32 po2size = MRT::nextPo2(std::max(xCount, yCount));

#if INVERT
    uint32 log2size = MRT::log2U32(po2size);
#endif
    
    uint32 index = 0;
    for (uint32 d = 0; d < po2size*po2size; d++) {
        uint32 x, y;

#if HILBERT
        hilbertDtoXY(po2size, d, &x, &y);
#else // Morton
        deInterleave(d, &x, &y);
#endif

#if INVERT
        // from http://www.tomgibara.com/computer-vision/minimizing-spatial-cohesion
        x = reverseU32(x) >> (32u - log2size);
        y = reverseU32(y) >> (32u - log2size);
#endif

        if ((x < xCount) && (y < yCount)) {
            worklistFinal[index++] = worklist[x + y * xCount];
        }
        if (index == numTiles)
            break;
    }
    free(worklist);
    worklist = worklistFinal;
#endif
}


//////////////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////////////

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