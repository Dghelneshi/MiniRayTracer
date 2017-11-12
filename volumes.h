#pragma once

#include "scene_object.h"
#include "aabb.h"
#include "material.h"
#include "texture.h"

class constant_volume : public scene_object {
public:
    scene_object *boundary;
    float density;
    material *phase_function;

    constant_volume(scene_object *boundary, float density, texture *albedo) : boundary(boundary), density(density) {
        phase_function = new isotropic(albedo);
    }
    bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const override;
    bool bounding_box(aabb *box, float time0, float time1) const override {
        return boundary->bounding_box(box, time0, time1);
    }
};
