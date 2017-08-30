#pragma once

#include "common.h"
#include "vec3.h"
#include "ray.h"
#include <algorithm> // std::swap


class aabb {
public:
    vec3 _min;
    vec3 _max;
    
    aabb() {}

    // a must be < b in all dimensions
    aabb(const vec3& a, const vec3& b) : _min(a), _max(b) {}

    bool hit(const ray& r, float tmin, float tmax) const {
        
        for (int axis = 0; axis < 3; axis++) {

            float invDir = 1.0f / r.dir[axis];

            float t0 = (_min[axis] - r.origin[axis]) * invDir;
            float t1 = (_max[axis] - r.origin[axis]) * invDir;

            if (invDir < 0.0f) {
                std::swap(t0, t1);
            }

            tmin = std::max(t0, tmin);
            tmax = std::min(t1, tmax);

            if (tmax < tmin)
                return false;
        }
        return true;
    }

};

aabb surrounding_box(const aabb& a, const aabb& b) {
    return aabb(vmin(a._min, b._min), vmax(a._max, b._max));
}