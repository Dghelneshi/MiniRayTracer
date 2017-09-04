#pragma once

#include "vec3.h"

class ray {
public:
    vec3 origin;
    vec3 dir;
    float time;

    ray(): time(0) {} ;
    
    // direction will be normalized
    ray(const vec3& origin, const vec3& dir, float time) { 
        this->origin = origin;
        this->dir = normalize(dir);
        this->time = time;
    }

    vec3 eval(float t) const {
        return origin + t * dir;
    }

};