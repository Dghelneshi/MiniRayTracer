#pragma once

#include "ray.h"
#include "pcg.h"

class camera {
public:
    Vec3 origin;
    Vec3 u, v, w; // x y z in camera space
    Vec3 llcorner;
    Vec3 horz;
    Vec3 vert;
    float lens_radius;
    float time0, time1; // shutter open/close times

    camera(const Vec3& pos, const Vec3& lookat, const Vec3& up, float vfov, float aspect, float aperture, float focus_dist, float shutter_t0, float shutter_t1) {

        time0 = shutter_t0;
        time1 = shutter_t1;

        float theta = RAD(vfov);
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

    ray get_ray(float s, float t) const {
        Vec3 rd = lens_radius * random_in_disk();
        Vec3 offset = u * rd.x + v * rd.y;
        float time = time0 + (time1 - time0) * randf();

        ray ray(origin + offset, llcorner + s * horz + t * vert - origin - offset, time);
        return ray;
    }
};