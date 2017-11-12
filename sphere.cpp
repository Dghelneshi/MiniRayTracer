#include "sphere.h"
#include "math.h"
#include <limits>

// gets uv for point on unit sphere (i.e. pass in normal for p)
inline void get_sphere_uv(const Vec3& p, float *u, float *v) {
    float phi = atan2(p.z, p.x);
    float theta = asin(p.y);
    *u = 0.5f - phi * (1.0f / (2.0f * M_PI_F));
    *v = 0.5f + theta * (1.0f / M_PI_F);
}

bool sphere::hit(const ray& r, float tmin, float tmax, hit_record *rec) const {

    rec->mat_ptr = mat_ptr;
    Vec3 cen = center(r.time);

    Vec3 oc = r.origin - cen;
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
            get_sphere_uv(rec->n, &rec->u, &rec->v);
            return true;
        }
        if (r.isInside) {
            // back
            t = (-b + sqrt(discriminant));
            if (t < tmax && t > tmin) {
                rec->t = t;
                rec->p = r.eval(t);
                rec->n = (rec->p - cen) / radius;
                get_sphere_uv(rec->n, &rec->u, &rec->v);
                return true;
            }
        }
    }
    return false;
}

bool sphere::bounding_box(aabb* box, float t0, float t1) const {

    float abs_r = MRT::abs(radius); // negative radius is allowed as hollow sphere, but bounding box must not be reversed as well!

    Vec3 r(abs_r, abs_r, abs_r);
    Vec3 c0 = center(t0);
    Vec3 c1 = center(t1);

    aabb bb0(c0 - r, c0 + r);
    aabb bb1(c1 - r, c1 + r);

    *box = surrounding_box(bb0, bb1);
    return true;
}

float sphere::pdf_value(const Vec3& origin, const Vec3& dir, float time) const {
    hit_record rec;
    if (this->hit(ray(origin, dir, time), 0.001f, std::numeric_limits<float>::max(), &rec)) {
        float cos_theta_max = sqrt(1 - radius * radius / sdot(center(time) - origin));
        float solid_angle = 2 * M_PI_F * (1 - cos_theta_max);
        return 1 / solid_angle;
    }
    else
        return 0;
}

Vec3 sphere::pdf_generate(const Vec3& origin, float time) const {
    Vec3 dir = center(time) - origin;
    float dist_sq = sdot(dir);
    onb uvw(normalize(dir));
    return uvw * random_towards_sphere(radius, dist_sq);
}