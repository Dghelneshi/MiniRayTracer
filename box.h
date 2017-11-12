#pragma once

#include "scene_object.h"
#include "rect.h"

class box final : public scene_object {
public:
    Vec3 min, max;
    object_list<scene_object> *rect_list;

    box() = default;
    box(const Vec3& min, const Vec3& max, material *mat) : min(min), max(max) {
        scene_object **list = new scene_object*[6];
        list[0] = new xy_rect(min.x, max.x, min.y, max.y, max.z, mat);
        list[1] = new xy_rect(max.x, min.x, min.y, max.y, min.z, mat);
        list[2] = new xz_rect(min.x, max.x, min.z, max.z, max.y, mat);
        list[3] = new xz_rect(max.x, min.x, min.z, max.z, min.y, mat);
        list[4] = new yz_rect(min.y, max.y, min.z, max.z, max.x, mat);
        list[5] = new yz_rect(max.y, min.y, min.z, max.z, min.x, mat);
        rect_list = new object_list<scene_object>(list, 6, 0, 0);
    }

    bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const override {
        return rect_list->hit(r, tmin, tmax, rec);
    }
    bool bounding_box(aabb* box, float time0, float time1) const override {
        *box = aabb(min, max);
        return true;
    }
    
};