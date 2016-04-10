#pragma once

#include "scene_object.h"
#include "rect.h"

class box : public scene_object {
public:
    vec3 min, max;
    object_list *rect_list;

    box() {}
    box(const vec3& min, const vec3& max, material *mat);
    virtual bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const {
        return rect_list->hit(r, tmin, tmax, rec);
    }
    virtual bool bounding_box(aabb* box, float time0, float time1) const {
        *box = aabb(min, max);
        return true;
    }
    
};

box::box(const vec3& min, const vec3& max, material *mat) {
    this->min = min;
    this->max = max;
    scene_object **list = new scene_object*[6];
    list[0] = new xy_rect(min.x, max.x, min.y, max.y, max.z, mat);
    list[1] = new xy_rect(max.x, min.x, min.y, max.y, min.z, mat);
    list[2] = new xz_rect(min.x, max.x, min.z, max.z, max.y, mat);
    list[3] = new xz_rect(max.x, min.x, min.z, max.z, min.y, mat);
    list[4] = new yz_rect(min.y, max.y, min.z, max.z, max.x, mat);
    list[5] = new yz_rect(max.y, min.y, min.z, max.z, min.x, mat);
    rect_list = new object_list(list, 6, 0, 0);
}