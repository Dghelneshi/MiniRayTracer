#include "rect.h"
#include <utility>
#include <limits>

// x0 > x1 XOR y0 > y1 flips the normal
xy_rect::xy_rect(float x0, float x1, float y0, float y1, float z, material *mat) : z(z), mat_ptr(mat) {

    normal_sign = 1;

    if (x0 > x1) {
        normal_sign *= -1;
        std::swap(x0, x1);
    }
    if (y0 > y1) {
        normal_sign *= -1;
        std::swap(y0, y1);
    }
    this->x0 = x0;
    this->x1 = x1;
    this->y0 = y0;
    this->y1 = y1;
}

bool xy_rect::hit(const ray& r, float tmin, float tmax, hit_record *rec) const {

    if (dot(r.dir, Vec3(0, 0, normal_sign)) > 0.0f)
        return false;

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
    rec->n = Vec3(0, 0, normal_sign);
    return true;
}

/////////////////////////////////////////


// x0 > x1 XOR z0 > z1 flips the normal
xz_rect::xz_rect(float x0, float x1, float z0, float z1, float y, material *mat) : y(y), mat_ptr(mat) {

    normal_sign = 1;

    if (x0 > x1) {
        normal_sign *= -1;
        std::swap(x0, x1);
    }
    if (z0 > z1) {
        normal_sign *= -1;
        std::swap(z0, z1);
    }
    this->x0 = x0;
    this->x1 = x1;
    this->z0 = z0;
    this->z1 = z1;
}

bool xz_rect::hit(const ray& r, float tmin, float tmax, hit_record *rec) const {

    if (dot(r.dir, Vec3(0, normal_sign, 0)) > 0.0f)
        return false;

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
    rec->n = Vec3(0, normal_sign, 0);
    return true;
}

float xz_rect::pdf_value(const Vec3& origin, const Vec3& dir, float time) const {
    hit_record rec;
    if (this->hit(ray(origin, dir, 0.0f), 0.001f, std::numeric_limits<float>::max(), &rec)) {
        float area = (x1 - x0) * (z1 - z0);
        float dist_sq = rec.t * rec.t;
        float cosine = MRT::abs(dot(dir, rec.n));
        return dist_sq / (cosine * area);
    }
    else
        return 0;
}

Vec3 xz_rect::pdf_generate(const Vec3& origin, float time) const {
    Vec3 rand = Vec3(x0 + randf() * (x1 - x0), y, z0 + randf() * (z1 - z0));
    return rand - origin;
}

/////////////////////////////////////////

// y0 > y1 XOR z0 > z1 flips the normal
yz_rect::yz_rect(float y0, float y1, float z0, float z1, float x, material *mat) : x(x), mat_ptr(mat) {

    normal_sign = 1;

    if (y0 > y1) {
        normal_sign *= -1;
        std::swap(y0, y1);
    }
    if (z0 > z1) {
        normal_sign *= -1;
        std::swap(z0, z1);
    }
    this->y0 = y0;
    this->y1 = y1;
    this->z0 = z0;
    this->z1 = z1;
}

bool yz_rect::hit(const ray& r, float tmin, float tmax, hit_record *rec) const {

    if (dot(r.dir, Vec3(normal_sign, 0, 0)) > 0.0f)
        return false;

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
    rec->n = Vec3(normal_sign, 0, 0);

    return true;
}