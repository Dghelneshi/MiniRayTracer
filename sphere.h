#pragma once

#include "scene_object.h"
#include "material.h"

class sphere : public scene_object {
public:
    vec3 center0, center1; // moing from center0 to center1
    float time0, time1;
    bool isMoving;

    float radius;
    material* mat_ptr;

    sphere() {}
    sphere(vec3 center0, float r, material *mat, vec3 center1 = { 0, 0, 0 }, float t0 = 0.0f, float t1 = 0.0f) : 
        center0(center0), center1(center1), time0(t0), time1(t1), radius(r), mat_ptr(mat) {
        
        isMoving = (time1 - time0) > FLT_EPSILON;
    };

    inline vec3 center(float time) const {
        if (isMoving) {
            return center0 + ((time - time0) / (time1 - time0)) * (center1 - center0);
        }
        else {
            return center0;
        }
    }

    virtual bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const;
    virtual bool bounding_box(aabb *box, float t0, float t1) const;
};

bool sphere::hit(const ray& r, float tmin, float tmax, hit_record *rec) const {

    rec->mat_ptr = mat_ptr;
    vec3 cen = center(r.time);
    
    vec3 oc = r.origin - cen;
    float b = dot(oc, r.dir);
    float c = sdot(oc) - radius * radius;
    float discriminant = b*b - c;

    if (discriminant > 0) {
        // front
        float t = (-b - sqrt(discriminant));
        if (t < tmax && t > tmin) {
            rec->t = t;
            rec->p = r.eval(t);
            rec->n = (rec->p - cen) / radius;
            return true;
        }
        // back
        t = (-b + sqrt(discriminant));
        if (t < tmax && t > tmin) {
            rec->t = t;
            rec->p = r.eval(t);
            rec->n = (rec->p - cen) / radius;
            return true;
        }
    }
    return false;
}

bool sphere::bounding_box(aabb* box, float t0, float t1) const {

    float abs_r = std::abs(radius); // negative radius is allowed as hollow sphere, but bounding box must not be reversed as well!

    vec3 r(abs_r, abs_r, abs_r);
    vec3 c0 = center(t0);
    vec3 c1 = center(t1);

    aabb bb0(c0 - r, c0 + r);
    aabb bb1(c1 - r, c1 + r);

    *box = surrounding_box(bb0, bb1);
    return true;
}


