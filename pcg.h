#pragma once

// *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)

#include <inttypes.h>

typedef struct { uint64_t state;  uint64_t inc; } pcg32_random_t;

uint32_t pcg32_random_r(pcg32_random_t* rng)
{
    uint64_t oldstate = rng->state;
    // Advance internal state
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    // Calculate output function (XSH RR), uses old state for max ILP
    uint32_t xorshifted = uint32_t(((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32_t rot = oldstate >> 59u;
#if _MSC_VER
    return _rotr(xorshifted, rot);
#else 
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
#endif
}

void pcg32_srandom_r(pcg32_random_t* rng, uint64_t initstate, uint64_t initseq)
{
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    pcg32_random_r(rng);
    rng->state += initstate;
    pcg32_random_r(rng);
}

///////////////////////////////////////////////////////////////////////////////

// convenience rng so we don't have to pass around pointers everywhere, pre-seeded so it can be used even before main is run
static pcg32_random_t G_rng = { 11350390909718046443uLL, 6305599193148252115uLL };

inline uint32_t rand32() {
    return pcg32_random_r(&G_rng);
}

// gets a random float in range [0,1)
float randf(pcg32_random_t* rng) {

    union a { float f; uint32_t bits; };

    a foo;
    foo.bits = 0x3f800000;
    foo.bits |= pcg32_random_r(rng) & 0x007FFFFF;

    return foo.f - 1.0f;
}
inline float randf() {
    return randf(&G_rng);
}

#include "vec3.h"


vec3 random_in_sphere(pcg32_random_t *rng) {
    vec3 p;
    do { // just try until we find one
        p = 2.0f * vec3(randf(rng), randf(rng), randf(rng)) - vec3(1, 1, 1);
    } while (sdot(p) >= 1.0f);

    return p;
}
inline vec3 random_in_sphere() {
    return random_in_sphere(&G_rng);
}


vec3 random_in_disk(pcg32_random_t *rng) {
    vec3 p;
    do { // just try until we find one
        p = 2.0f * vec3(randf(rng), randf(rng), 0) - vec3(1, 1, 0);
    } while (sdot(p) >= 1.0f);

    return p;
}
inline vec3 random_in_disk() {
    return random_in_disk(&G_rng);
}