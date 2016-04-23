#pragma once

#include "vec3.h"
#include <cmath>


class onb {
public:
    union {
        struct {
            vec3 u;
            vec3 v;
            vec3 w;
        };
        vec3 axis[3];
    };

    onb() {}
    onb(const vec3& n) { // builds ONB from w = n
        w = unit_vector(n);
        vec3 a;
        if (std::abs(w.x) > 0.9f)
            a = vec3(0, 1, 0);
        else
            a = vec3(1, 0, 0);
        v = cross(w, a).normalize();
        u = cross(w, v);
    }

    vec3 operator*(const vec3 &vec) const {
        return vec.x*u + vec.y*v + vec.z*w;
    }
    friend vec3 operator*(const vec3 &vec, const onb &o) {
        return vec.x*o.u + vec.y*o.v + vec.z*o.w;
    }
};