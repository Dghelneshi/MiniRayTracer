#include <limits>
#include <math.h>
#include "scene_object.h"

/////////////////////////
//     TRANSLATION     //
/////////////////////////

bool translate::hit(const ray& r, float tmin, float tmax, hit_record *rec) const {
    ray moved_ray(r.origin - offset, r.dir, r.time);

    if (obj->hit(moved_ray, tmin, tmax, rec)) {
        rec->p += offset;
        return true;
    }
    else
        return false;
}

bool translate::bounding_box(aabb *box, float time0, float time1) const {
    if (obj->bounding_box(box, time0, time1)) {
        *box = aabb(box->min + offset, box->max + offset);
        return true;
    }
    else
        return false;
}

////////////////////////
//      ROTATION      //
////////////////////////

rotate_y::rotate_y(scene_object *o, float angle) {
    obj = o;
    float radians = RAD(angle);
    sin_theta = sin(radians);
    cos_theta = cos(radians);
    hasBox = obj->bounding_box(&bbox, 0, 1);

    if (!hasBox) {
        bbox = aabb(Vec3(1, 1, 1), Vec3(-1, -1, -1));
        return;
    }

    Vec3 minbb(std::numeric_limits<float>::max(),    std::numeric_limits<float>::max(),    std::numeric_limits<float>::max());
    Vec3 maxbb(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            for (int k = 0; k < 2; k++)
            {
                float x = i * bbox.max.x + (1 - i) * bbox.min.x;
                float y = j * bbox.max.y + (1 - j) * bbox.min.y;
                float z = k * bbox.max.z + (1 - k) * bbox.min.z;

                float newx = cos_theta*x + sin_theta*z;
                float newz = cos_theta*z - sin_theta*x;

                Vec3 testvec(newx, y, newz);
                minbb = vmin(minbb, testvec);
                maxbb = vmax(maxbb, testvec);
            }
        }
    }
    bbox = aabb(minbb, maxbb);
}

bool rotate_y::hit(const ray& r, float tmin, float tmax, hit_record *rec) const {

    if (hasBox && !bbox.hit(r, tmin, tmax)) // it's faster to test against bbox first before rotating
        return false;

    Vec3 origin = r.origin;
    Vec3 dir = r.dir;
    origin.x = cos_theta * r.origin.x - sin_theta * r.origin.z;
    origin.z = cos_theta * r.origin.z + sin_theta * r.origin.x;
    dir.x = cos_theta * r.dir.x - sin_theta * r.dir.z;
    dir.z = cos_theta * r.dir.z + sin_theta * r.dir.x;

    ray rotated_ray(origin, dir, r.time);

    if (obj->hit(rotated_ray, tmin, tmax, rec)) {
        Vec3 p = rec->p;
        Vec3 n = rec->n;
        p.x = cos_theta * rec->p.x + sin_theta * rec->p.z;
        p.z = cos_theta * rec->p.z - sin_theta * rec->p.x;

        n.x = cos_theta * rec->n.x + sin_theta * rec->n.z;
        n.z = cos_theta * rec->n.z - sin_theta * rec->n.x;
        rec->p = p;
        rec->n = n;
        return true;
    }
    else
        return false;
}