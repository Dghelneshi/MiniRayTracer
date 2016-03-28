#pragma once

#include "ray.h"
#include "pcg.h"
#define _USE_MATH_DEFINES
#include "math.h" // M_PI

// for clang
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class camera {
public:
    vec3 origin;
    vec3 u, v, w; // x y z in camera space
    vec3 llcorner;
    vec3 horz;
    vec3 vert;
    float lens_radius;

    camera(const vec3& pos, const vec3& lookat, const vec3& up, float vfov, float aspect, float aperture, float focus_dist) {

        float theta = vfov * float(M_PI) / 180.0f;
        float height = 2.0f * tan(theta / 2);
        float width = aspect * height;

        origin = pos;
        w = (pos - lookat).normalize();
        u = cross(up, w).normalize();
        v = cross(w, u);

        lens_radius = aperture / 2.0f;

        horz = focus_dist * width * u;
        vert = focus_dist * height * v;
        llcorner = origin - 0.5f * horz - 0.5f * vert - focus_dist*w;

    }

    ray get_ray(float s, float t, pcg32_random_t *rng) {
        vec3 rd = lens_radius * random_in_disk(rng);
        vec3 offset = u * rd.x + v * rd.y;
        ray ray(origin + offset, llcorner + s * horz + t * vert - origin - offset);
        return ray;
    }
};