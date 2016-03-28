#pragma once

#include "ray.h"

class material;

struct hit_record {
    float t;
    vec3 p;
    vec3 n;
    material *mat_ptr;
};

// abstract base class
class scene_object {
public:
    virtual bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const = 0;
    virtual ~scene_object() {}
};


//////////////////////////////////////////////////////////////////////////////////////


class object_list : public scene_object {
public:
    scene_object **list;
    int count;

    object_list() {}
    object_list(scene_object **l, int n) : list(l), count(n) {};

    virtual bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const;

};

bool object_list::hit(const ray& r, float tmin, float tmax, hit_record *rec) const {
    
    hit_record cur_rec;
    bool hit = false;
    float closest = tmax;

    for (int i = 0; i < count; i++) {
        
        if (list[i]->hit(r, tmin, closest, &cur_rec)) {
            hit = true;
            closest = cur_rec.t;
            *rec = cur_rec;
        }
    }

    return hit;

}