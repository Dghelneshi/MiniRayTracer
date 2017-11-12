#if _MSC_VER
#include <intrin.h> // _rotr()
#endif
#include "pcg.h"

// *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)

#include <inttypes.h>
typedef struct { uint64 state;  uint64 inc; } pcg32_random_t;

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

// convenience rng for static initialization, pre-seeded so it can be used even before main is run
static pcg32_random_t G_rng = { 11350390909718046443uLL, 6305599193148252115uLL };

thread_local pcg32_random_t T_rng; // threadsafe global RNG

void Init_Thread_RNG(uint64 initstate, uint64 initseq) {
    pcg32_srandom_r(&T_rng, initstate, initseq);
}

uint32_t rand32() {
    return pcg32_random_r(&T_rng);
}

// gets a random float in range [0,1)
float randf(pcg32_random_t* rng) {

    union a { float f; uint32_t bits; };

    a foo;
    foo.bits = 0x3f800000;
    foo.bits |= pcg32_random_r(rng) & 0x007FFFFF;

    return foo.f - 1.0f;
}
float randf() {
    return randf(&T_rng);
}
float randf_g() {
    return randf(&G_rng);
}

Vec3 random_in_sphere(pcg32_random_t *rng) {
    Vec3 p;
    do { // just try until we find one
        p = 2.0f * Vec3(randf(rng), randf(rng), randf(rng)) - Vec3(1, 1, 1);
    } while (sdot(p) >= 1.0f);

    return p;
}
Vec3 random_in_sphere() {
    return random_in_sphere(&T_rng);
}
Vec3 random_in_sphere_g() {
    return random_in_sphere(&G_rng);
}

// NOTE: This is not normalized!
// It probably should be, but the only place we use this is for generating new rays, which normalize in the constructor.
Vec3 random_cosine_direction(pcg32_random_t *rng) {
    float r1 = randf(rng);
    float r2 = randf(rng);
    float z = sqrt(1 - r2);
    float phi = 2 * M_PI_F * r1;
    float x = cos(phi) * 2 * sqrt(r2);
    float y = sin(phi) * 2 * sqrt(r2);
    return Vec3(x, y, z);
}
Vec3 random_cosine_direction() {
    return random_cosine_direction(&T_rng);
}

Vec3 random_on_sphere_uniform(pcg32_random_t *rng) {
    float x = randf(rng) * 2 - 1.0f;
    float phi = randf(rng) * 2 * M_PI_F;
    float s = sqrt(1 - x*x);
    float y = cos(phi) * s;
    float z = sin(phi) * s;
    return Vec3(x, y, z);
}
Vec3 random_on_sphere_uniform() {
    return random_on_sphere_uniform(&T_rng);
}

Vec3 random_in_disk(pcg32_random_t *rng) {
    Vec3 p;
    do { // just try until we find one
        p = 2.0f * Vec3(randf(rng), randf(rng), 0) - Vec3(1, 1, 0);
    } while (sdot(p) >= 1.0f);

    return p;
}
Vec3 random_in_disk() {
    return random_in_disk(&T_rng);
}


Vec3 random_towards_sphere(float radius, float dist_sq, pcg32_random_t *rng) {
    float r1 = randf(rng);
    float r2 = randf(rng);
    float z = 1 + r2 * (sqrt(1 - radius*radius / dist_sq) - 1);
    float phi = 2 * M_PI_F * r1;
    float x = cos(phi) * sqrt(1 - z*z);
    float y = sin(phi) * sqrt(1 - z*z);
    return Vec3(x, y, z);
}
Vec3 random_towards_sphere(float radius, float dist_sq) {
    return random_towards_sphere(radius, dist_sq, &T_rng);
}