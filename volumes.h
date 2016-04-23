#pragma once

#include "scene_object.h"
#include "math.h" // log

class constant_volume : public scene_object {
public:
    scene_object *boundary;
    float density;
    material *phase_function;

    constant_volume(scene_object *boundary, float density, texture *albedo) : boundary(boundary), density(density) {
        phase_function = new isotropic(albedo);
    }
    virtual bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const override;
    virtual bool bounding_box(aabb *box, float time0, float time1) const override {
        return boundary->bounding_box(box, time0, time1);
    }
};

bool constant_volume::hit(const ray& r, float tmin, float tmax, hit_record *rec) const {
    
    /*
        rec1 contains minimum hit and rec2 contains maximum hit (i,e. opposite side of the boundary)
        if we are inside the volume, rec1 will hit behind us, rec2 will hit in front
    */
    hit_record rec1, rec2;

    if (boundary->hit(r, -FLT_MAX, FLT_MAX, &rec1)) {
        if (boundary->hit(r, rec1.t + 0.0001f, FLT_MAX, &rec2)) {

            if (rec1.t < tmin)
                rec1.t = tmin;
            if (rec2.t > tmax)
                rec2.t = tmax;
            if (rec1.t >= rec2.t)
                return false;
            if (rec1.t < 0)
                rec1.t = 0;

            float inside_dist = (rec2.t - rec1.t);
            float hit_dist = -(1 / density) * log(randf()); // randf() is not thread safe, but I don't want to pass around rng pointer everywhere

            if (hit_dist < inside_dist) {
                rec->t = rec1.t + hit_dist;
                rec->p = r.eval(rec->t);
                rec->n = vec3(1, 0, 0); // arbitrary
                rec->mat_ptr = phase_function;
                return true;
            }
        }
    }
    return false;
}