#pragma once

#include "ray.h"
#include "pcg.h"

class camera {
public:
    vec3 origin;
    vec3 u, v, w; // x y z in camera space
    vec3 llcorner;
    vec3 horz;
    vec3 vert;
    float lens_radius;
    float time0, time1; // shutter open/close times

    camera(const vec3& pos, const vec3& lookat, const vec3& up, float vfov, float aspect, float aperture, float focus_dist, float shutter_t0, float shutter_t1) {

        time0 = shutter_t0;
        time1 = shutter_t1;

        float theta = vfov * float(M_PI_F) / 180.0f;
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
        float time = time0 + (time1 - time0) * randf(rng);

        ray ray(origin + offset, llcorner + s * horz + t * vert - origin - offset, time);
        return ray;
    }
};