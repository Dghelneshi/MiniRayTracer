#pragma once

#include "scene_object.h"
#include "material.h"

class sphere : public scene_object {
public:
    vec3 center;
    float radius;
    material* mat_ptr;

    sphere() {}
    sphere(vec3 c, float r, material *mat) : center(c), radius(r), mat_ptr(mat) {};

    virtual bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const;
};

bool sphere::hit(const ray& r, float tmin, float tmax, hit_record *rec) const {

    rec->mat_ptr = mat_ptr;
    
    vec3 oc = r.origin - center;
    //float a = sdot(r.dir);
    float b = dot(oc, r.dir);
    float c = sdot(oc) - radius * radius;
    float discriminant = b*b - c;

    if (discriminant > 0) {
        // front
        float t = (-b - sqrt(discriminant));
        if (t < tmax && t > tmin) {
            rec->t = t;
            rec->p = r.eval(t);
            rec->n = (rec->p - center) / radius;
            return true;
        }
        // back
        t = (-b + sqrt(discriminant));
        if (t < tmax && t > tmin) {
            rec->t = t;
            rec->p = r.eval(t);
            rec->n = (rec->p - center) / radius;
            return true;
        }
    }
    return false;
}
