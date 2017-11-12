#pragma once

#include "common.h"
#include "vec3.h"
#include "ray.h"
#include <algorithm> // std::swap, std::min/max


class aabb {
public:
    Vec3 min;
    Vec3 max;
    
    aabb() {}

    // min must be < max in all dimensions
    aabb(const Vec3& min, const Vec3& max) : min(min), max(max) {}

    bool hit(const ray& r, float tmin, float tmax) const {
#if 0
        for (size_t axis = 0; axis < 3; axis++) {

            float invDir = 1.0f / r.dir[axis];

            float t0 = (min[axis] - r.origin[axis]) * invDir;
            float t1 = (max[axis] - r.origin[axis]) * invDir;

            if (invDir < 0.0f) {
                std::swap(t0, t1);
            }

            tmin = std::max(t0, tmin);
            tmax = std::min(t1, tmax);

            if (tmax < tmin)
                return false;
        }
        return true;
#else
        Vec3 invDir = 1.0f / r.dir;

        Vec3 t0 = (min - r.origin) * invDir;
        Vec3 t1 = (max - r.origin) * invDir;

        // exchange t0[i] and t1[i] if invDir[i] < 0
        __m128 ltZero = _mm_cmplt_ps(invDir.m, _mm_setzero_ps());
        m128 t0n = _mm_blendv_ps(t0.m, t1.m, ltZero);
        m128 t1n = _mm_blendv_ps(t1.m, t0.m, ltZero);

        // shift vector elements down
        m128 t0n1 = _mm_permute_ps(t0n, _MM_SHUFFLE(0, 0, 0, Y));
        m128 t0n2 = _mm_permute_ps(t0n, _MM_SHUFFLE(0, 0, 0, Z));
        m128 t1n1 = _mm_permute_ps(t1n, _MM_SHUFFLE(0, 0, 0, Y));
        m128 t1n2 = _mm_permute_ps(t1n, _MM_SHUFFLE(0, 0, 0, Z));

        // do a successive max/min with tmin/tmax and every element of t0/t1
        m128 tminv = _mm_max_ss(t0n, _mm_set_ss(tmin));
        m128 tmaxv = _mm_min_ss(t1n, _mm_set_ss(tmax));
        tminv = _mm_max_ss(t0n1, tminv);
        tminv = _mm_max_ss(t0n2, tminv);
        tmaxv = _mm_min_ss(t1n1, tmaxv);
        tmaxv = _mm_min_ss(t1n2, tmaxv);

        return (tmaxv.f32[0] > tminv.f32[0]);
#endif
    }

};

inline aabb surrounding_box(const aabb& a, const aabb& b) {
    return aabb(vmin(a.min, b.min), vmax(a.max, b.max));
}