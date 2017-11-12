#pragma once

#include "scene_object.h"
#include "vec3.h"

#ifdef NEW_INTERSECT
//#define BACKFACE_CULLING
#endif

class triangle final : public scene_object {
public:
#ifdef NEW_INTERSECT
    Vec3 a, b, c;
    Vec3 an, bn, cn;
#else
    Vec3 m, u, v;
    Vec3 mn, un, vn;
#endif
    material *mat_ptr;
    
    triangle(const Vec3 &a, const Vec3 &b, const Vec3 &c, material *mat);
    triangle(const Vec3 &a, const Vec3 &b, const Vec3 &c, const Vec3& an, const Vec3& bn, const Vec3& cn, material *mat);

    bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const override;
    bool bounding_box(aabb* box, float time0, float time1) const override;
};
