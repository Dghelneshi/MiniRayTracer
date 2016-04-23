#pragma once

#include "scene_object.h"
#include "material.h"

class xy_rect : public scene_object {
public:
    float x0, x1, y0, y1;
    float z;
    material* mat_ptr;
    int normal_multiplier;

    xy_rect() {}
    // x0 > x1 XOR y0 > y1 flips the normal
    xy_rect(float x0, float x1, float y0, float y1, float z, material *mat) : z(z), mat_ptr(mat) {
        
        normal_multiplier = 1;

        if (x0 > x1) {
            normal_multiplier *= -1;
            std::swap(x0, x1);
        }
        if (y0 > y1) {
            normal_multiplier *= -1;
            std::swap(y0, y1);
        }
        this->x0 = x0;
        this->x1 = x1;
        this->y0 = y0;
        this->y1 = y1;
    }

    virtual bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const override;
    virtual bool bounding_box(aabb* box, float time0, float time1) const override {
        *box = aabb(vec3(x0, y0, z - 0.0001f), vec3(x1, y1, z + 0.0001f)); // assumes x0 < x1, y0 < y1
        return true;
    }
};

bool xy_rect::hit(const ray& r, float tmin, float tmax, hit_record *rec) const {
    
    float t = (z - r.origin.z) / r.dir.z;
    if (t < tmin || t > tmax)
        return false;

    float x = r.origin.x + t * r.dir.x;
    float y = r.origin.y + t * r.dir.y;
    if (x < x0 || x > x1 || y < y0 || y > y1)
        return false;

    rec->u = (x - x0) / (x1 - x0);
    rec->v = (y - y0) / (y1 - y0);
    rec->t = t;
    rec->mat_ptr = mat_ptr;
    rec->p = r.eval(t);
    rec->n = vec3(0.0f, 0.0f, 1.0f) * float(normal_multiplier);
    return true;
}

/////////////////////////////////////////

class xz_rect : public scene_object {
public:
    float x0, x1, z0, z1;
    float y;
    material* mat_ptr;
    int normal_multiplier;

    xz_rect() {}
    // x0 > x1 XOR z0 > z1 flips the normal
    xz_rect(float x0, float x1, float z0, float z1, float y, material *mat) : y(y), mat_ptr(mat) {

        normal_multiplier = 1;

        if (x0 > x1) {
            normal_multiplier *= -1;
            std::swap(x0, x1);
        }
        if (z0 > z1) {
            normal_multiplier *= -1;
            std::swap(z0, z1);
        }
        this->x0 = x0;
        this->x1 = x1;
        this->z0 = z0;
        this->z1 = z1;
    }

    virtual bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const override;
    virtual bool bounding_box(aabb* box, float time0, float time1) const override {
        *box = aabb(vec3(x0, y - 0.0001f, z0), vec3(x1, y + 0.0001f, z1)); // assumes x0 < x1, z0 < z1
        return true;
    }
    virtual float pdf_value(const vec3& origin, const vec3& dir, float time) const override;
    virtual vec3 pdf_generate(const vec3& origin, float time, pcg32_random_t *rng) const override;
};

bool xz_rect::hit(const ray& r, float tmin, float tmax, hit_record *rec) const {

    float t = (y - r.origin.y) / r.dir.y;
    if (t < tmin || t > tmax)
        return false;

    float x = r.origin.x + t * r.dir.x;
    float z = r.origin.z + t * r.dir.z;
    if (x < x0 || x > x1 || z < z0 || z > z1)
        return false;

    rec->u = (x - x0) / (x1 - x0);
    rec->v = (z - z0) / (z1 - z0);
    rec->t = t;
    rec->mat_ptr = mat_ptr;
    rec->p = r.eval(t);
    rec->n = vec3(0.0f, 1.0f, 0.0f) * float(normal_multiplier);
    return true;
}

float xz_rect::pdf_value(const vec3& origin, const vec3& dir, float time) const {
    hit_record rec;
    if (this->hit(ray(origin, dir, 0.0f), 0.001f, FLT_MAX, &rec)) {
        float area = (x1 - x0) * (z1 - z0);
        float dist_sq = rec.t * rec.t;
        float cosine = std::abs(dot(dir, rec.n));
        return dist_sq / (cosine * area);
    }
    else
        return 0;
}

vec3 xz_rect::pdf_generate(const vec3& origin, float time, pcg32_random_t *rng) const {
    vec3 rand = vec3(x0 + randf(rng) * (x1 - x0), y, z0 + randf() * (z1 - z0));
    return rand - origin;
}

/////////////////////////////////////////

class yz_rect : public scene_object {
public:
    float y0, y1, z0, z1;
    float x;
    material* mat_ptr;
    int normal_multiplier;

    yz_rect() {}
    // y0 > y1 XOR z0 > z1 flips the normal
    yz_rect(float y0, float y1, float z0, float z1, float x, material *mat) : x(x), mat_ptr(mat) {

        normal_multiplier = 1;

        if (y0 > y1) {
            normal_multiplier *= -1;
            std::swap(y0, y1);
        }
        if (z0 > z1) {
            normal_multiplier *= -1;
            std::swap(z0, z1);
        }
        this->y0 = y0;
        this->y1 = y1;
        this->z0 = z0;
        this->z1 = z1;
    }

    virtual bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const override;
    virtual bool bounding_box(aabb* box, float time0, float time1) const override {
        *box = aabb(vec3(x - 0.0001f, y0, z0), vec3(x + 0.0001f, y1, z1)); // assumes y0 < y1, z0 < z1
        return true;
    }
};

bool yz_rect::hit(const ray& r, float tmin, float tmax, hit_record *rec) const {

    float t = (x - r.origin.x) / r.dir.x;
    if (t < tmin || t > tmax)
        return false;

    float y = r.origin.y + t * r.dir.y;
    float z = r.origin.z + t * r.dir.z;
    if (y < y0 || y > y1 || z < z0 || z > z1)
        return false;

    rec->u = (y - y0) / (y1 - y0);
    rec->v = (z - z0) / (z1 - z0);
    rec->t = t;
    rec->mat_ptr = mat_ptr;
    rec->p = r.eval(t);
    rec->n = vec3(1.0f, 0.0f, 0.0f) * float(normal_multiplier);

    return true;
}