#pragma once

#include "common.h"
#include "vec3.h"

void Init_Thread_RNG(uint64 initstate, uint64 initseq);

uint32_t rand32();
float randf(); // gets a random float in range [0,1)
Vec3 random_in_sphere();

Vec3 random_on_sphere_uniform();
Vec3 random_in_disk();
Vec3 random_towards_sphere(float radius, float dist_sq);
// NOTE: This is not normalized!
// It probably should be, but the only place we use this is for generating new rays, which normalize in the constructor.
Vec3 random_cosine_direction();

// global RNG, for static initialization
float randf_g();
Vec3 random_in_sphere_g();