#pragma once

#include "scene_object.h"
#include "material.h"
#include "common.h"
#include "pcg.h"
#include <limits>

class sphere final : public scene_object {
public:
    Vec3 center0, center1; // moing from center0 to center1
    float time0, time1;
    bool isMoving;

    float radius;
    material* mat_ptr;

    sphere(Vec3 center0, float r, material *mat, Vec3 center1 = { 0, 0, 0 }, float t0 = 0.0f, float t1 = 0.0f) : 
        center0(center0), center1(center1), time0(t0), time1(t1), radius(r), mat_ptr(mat) {
        
        isMoving = (time1 - time0) > std::numeric_limits<float>::epsilon();
    };

    inline Vec3 center(float time) const {
        if (isMoving) {
            return center0 + ((time - time0) / (time1 - time0)) * (center1 - center0);
        }
        else {
            return center0;
        }
    }

    bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const override;
    bool bounding_box(aabb *box, float t0, float t1) const override;
    float pdf_value(const Vec3& origin, const Vec3& dir, float time) const override;
    Vec3 pdf_generate(const Vec3& origin, float time) const override;
};

