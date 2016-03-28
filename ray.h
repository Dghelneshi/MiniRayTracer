#pragma once

#include "vec3.h"

// direction is normalized
class ray {
public:
    vec3 origin;
    vec3 dir;

    ray() {};
    
    ray(const vec3& origin, const vec3& dir) { 
        this->origin = origin;
        this->dir = dir;
        this->dir.normalize();
    }

    vec3 eval(float t) const {
        return origin + t * dir;
    }

};