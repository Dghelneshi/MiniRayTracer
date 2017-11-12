#pragma once

#include "scene_object.h"
#include "material.h"

class xy_rect final : public scene_object {
public:
    float x0, x1, y0, y1;
    float z;
    material* mat_ptr;
    float normal_sign;

    xy_rect() = default;
    // x0 > x1 XOR y0 > y1 flips the normal
    xy_rect(float x0, float x1, float y0, float y1, float z, material *mat);

    bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const override;
    bool bounding_box(aabb* box, float time0, float time1) const override {
        *box = aabb(Vec3(x0, y0, z - 0.0001f), Vec3(x1, y1, z + 0.0001f)); // assumes x0 < x1, y0 < y1
        return true;
    }
};

/////////////////////////////////////////

class xz_rect final : public scene_object {
public:
    float x0, x1, z0, z1;
    float y;
    material* mat_ptr;
    float normal_sign;

    xz_rect() = default;
    // x0 > x1 XOR z0 > z1 flips the normal
    xz_rect(float x0, float x1, float z0, float z1, float y, material *mat);

    bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const override;
    bool bounding_box(aabb* box, float time0, float time1) const override {
        *box = aabb(Vec3(x0, y - 0.0001f, z0), Vec3(x1, y + 0.0001f, z1)); // assumes x0 < x1, z0 < z1
        return true;
    }
    float pdf_value(const Vec3& origin, const Vec3& dir, float time) const override;
    Vec3 pdf_generate(const Vec3& origin, float time) const override;
};


/////////////////////////////////////////

class yz_rect final : public scene_object {
public:
    float y0, y1, z0, z1;
    float x;
    material* mat_ptr;
    float normal_sign;

    yz_rect() = default;
    // y0 > y1 XOR z0 > z1 flips the normal
    yz_rect(float y0, float y1, float z0, float z1, float x, material *mat);

    bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const override;
    bool bounding_box(aabb* box, float time0, float time1) const override {
        *box = aabb(Vec3(x - 0.0001f, y0, z0), Vec3(x + 0.0001f, y1, z1)); // assumes y0 < y1, z0 < z1
        return true;
    }
};
