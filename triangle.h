#pragma once

#include "scene_object.h"

class triangle : public scene_object {
public:
    vec3 m, u, v;
    vec3 mn, un, vn;
    material *mat_ptr;
    
    triangle() {}
    triangle(const vec3 &a, const vec3 &b, const vec3 &c, material *mat);
    triangle(const vec3 &a, const vec3 &b, const vec3 &c, const vec3& an, const vec3& bn, const vec3& cn, material *mat);

    virtual bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const override;
    virtual bool bounding_box(aabb* box, float time0, float time1) const override {
        vec3 a = m;
        vec3 b = m + u;
        vec3 c = m + v;
        *box = aabb(vmin(a, b, c), vmax(a, b, c));
        return true;
    }
};

triangle::triangle(const vec3 &a, const vec3 &b, const vec3 &c, material *mat) {
    m = a;
    u = b - a;
    v = c - a;
    mn = un = vn = cross(u, v).normalize();
    mat_ptr = mat;
}

triangle::triangle(const vec3 &a, const vec3 &b, const vec3 &c, const vec3& an, const vec3& bn, const vec3& cn, material *mat) {
    m = a;
    u = b - a;
    v = c - a;
    mn = an;
    un = bn;
    vn = cn;
    mat_ptr = mat;
}

#define TRI_EPS 0.00001f

bool triangle::hit(const ray& r, float tmin, float tmax, hit_record *rec) const {

    vec3 pvec = cross(r.dir, v);
    float det = dot(u, pvec);

    // if the determinant is negative the triangle is backfacing
    // if the determinant is close to 0, the ray misses the triangle
    if (det < TRI_EPS)
        return false;

    float invDet = 1 / det;

    vec3 tvec = r.origin - m;
    float uu = dot(tvec, pvec) * invDet;
    if (uu < 0 || uu > 1)
        return false;

    vec3 qvec = cross(tvec, u);
    float vv = dot(r.dir, qvec) * invDet;
    if (vv < 0 || (uu + vv) > 1)
        return false;

    float t = dot(v, qvec) * invDet;

    if (t < tmin || t > tmax)
        return false;

    rec->t = t;
    rec->p = r.eval(t);
    rec->n = ((mn * (1 - uu - vv)) + (un * uu) + (vn * vv)).normalize();
    rec->u = uu;
    rec->v = vv;
    rec->mat_ptr = mat_ptr;

    return true;
}