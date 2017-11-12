#pragma once

#include "vec3.h"
#include "common.h"
#include "mrt_math.h"

class onb {
public:
    union {
        struct {
            Vec3 u;
            Vec3 v;
            Vec3 w;
        };
        Vec3 axis[3];
    };

    onb() {}
    onb(const Vec3& n) : w(n) { // builds ONB from w = n, n is assumed to be normalized!
        Vec3 a = (MRT::abs(w.x) > 0.9f) ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
        v = cross(w, a).normalize();
        u = cross(w, v);
    }

    Vec3 operator*(const Vec3 &vec) const {
        return vec.x*u + vec.y*v + vec.z*w;
    }
    friend Vec3 operator*(const Vec3 &vec, const onb &o) {
        return vec.x*o.u + vec.y*o.v + vec.z*o.w;
    }
};