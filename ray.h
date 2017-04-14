#pragma once

#include "vec3.h"

// direction is normalized
class ray {
public:
    vec3 origin;
    vec3 dir;
    float time;

    ray(): time(0) {} ;
    
    ray(const vec3& origin, const vec3& dir, float time) { 
        this->origin = origin;
        this->dir = dir;
        this->dir.normalize();
        this->time = time;
    }

    vec3 eval(float t) const {
        return origin + t * dir;
    }

};