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
#elif 0
        Vec3 invDir = 1.0f / r.dir;

        m128 t0 = ((min - r.origin) * invDir).m;
        m128 t1 = ((max - r.origin) * invDir).m;

        // exchange t0[i] and t1[i] if invDir[i] < 0
        __m128 ltZero = _mm_cmplt_ps(invDir.m, _mm_setzero_ps());
        t0 = _mm_blendv_ps(t0, t1, ltZero);
        t1 = _mm_blendv_ps(t1, t0, ltZero);

        // insert tmin and tmax as W component of t0/t1,
        t0 = _mm_insert_ps(t0, _mm_set_ss(tmin), 3 << 4);
        t1 = _mm_insert_ps(t1, _mm_set_ss(tmax), 3 << 4);
        
        // shift upper two elements down
        m128 t0zw = _mm_permute_ps(t0, _MM_SHUFFLE(0, 0, W, Z));
        m128 t1zw = _mm_permute_ps(t1, _MM_SHUFFLE(0, 0, W, Z));

        // do min/max of xy with zw
        m128 tminv = _mm_max_ps(t0, t0zw);
        m128 tmaxv = _mm_min_ps(t1, t1zw);
        // do another min/max of the two results from above
        tminv = _mm_max_ps(tminv, _mm_permute_ps(tminv, _MM_SHUFFLE(0, 0, 0, Y)));
        tmaxv = _mm_min_ps(tmaxv, _mm_permute_ps(tmaxv, _MM_SHUFFLE(0, 0, 0, Y)));

        return (tmaxv.f32[0] > tminv.f32[0]);
#else
        Vec3 invDir = 1.0f / r.dir;

        m128 t0 = ((min - r.origin) * invDir).m;
        m128 t1 = ((max - r.origin) * invDir).m;

        // exchange t0[i] and t1[i] if invDir[i] < 0
        __m128 ltZero = _mm_cmplt_ps(invDir.m, _mm_setzero_ps());
        t0 = _mm_blendv_ps(t0, t1, ltZero);
        t1 = _mm_blendv_ps(t1, t0, ltZero);

        // shift individual vector elements down
        m128 t0y = _mm_permute_ps(t0, _MM_SHUFFLE(0, 0, 0, Y));
        m128 t0z = _mm_permute_ps(t0, _MM_SHUFFLE(0, 0, 0, Z));
        m128 t1y = _mm_permute_ps(t1, _MM_SHUFFLE(0, 0, 0, Y));
        m128 t1z = _mm_permute_ps(t1, _MM_SHUFFLE(0, 0, 0, Z));

        // do a successive max/min with tmin/tmax and every element of t0/t1
        m128 tminv = _mm_max_ss(t0, _mm_set_ss(tmin));
        m128 tmaxv = _mm_min_ss(t1, _mm_set_ss(tmax));
        m128 maxyz = _mm_max_ss(t0y, t0z);
        tminv = _mm_max_ss(maxyz, tminv);
        m128 minyz = _mm_min_ss(t1y, t1z);
        tmaxv = _mm_min_ss(minyz, tmaxv);

        return (tmaxv.f32[0] > tminv.f32[0]);
#endif
    }

};

inline aabb surrounding_box(const aabb& a, const aabb& b) {
    return aabb(vmin(a.min, b.min), vmax(a.max, b.max));
}