#include "common.h"
#include "triangle.h"
#include "vec3.h"

triangle::triangle(const Vec3 &a, const Vec3 &b, const Vec3 &c, material *mat) : mat_ptr(mat) {
#ifdef NEW_INTERSECT
    this->a = a;
    this->b = b;
    this->c = c;
    an = bn = cn = cross(b - a, c - a).normalize();
#else
    m = a;
    u = b - a;
    v = c - a;
    mn = un = vn = cross(u, v).normalize();
#endif
}

triangle::triangle(const Vec3 &a, const Vec3 &b, const Vec3 &c, const Vec3& an, const Vec3& bn, const Vec3& cn, material *mat) : mat_ptr(mat) {
#ifdef NEW_INTERSECT
    this->a = a;
    this->b = b;
    this->c = c;
    this->an = an;
    this->bn = bn;
    this->cn = cn;
#else
    m = a;
    u = b - a;
    v = c - a;
    mn = an;
    un = bn;
    vn = cn;
#endif
}

bool triangle::bounding_box(aabb* box, float time0, float time1) const {
#ifndef NEW_INTERSECT
    Vec3 a = m;
    Vec3 b = m + u;
    Vec3 c = m + v;
#endif
    *box = aabb(vmin(a, b, c), vmax(a, b, c));
    return true;
}

#define TRI_EPS 0.00001f

bool triangle::hit(const ray& r, float tmin, float tmax, hit_record *rec) const {
#ifndef NEW_INTERSECT
    Vec3 pvec = cross(r.dir, v);
    float det = dot(u, pvec);

    float sign = 1.0f;
    if (r.isInside) {
        sign = det < 0.0f ? -1.0f : 1.0f;
        det = sign * det;
    }

    // if the determinant is negative the triangle is backfacing
    // if the determinant is close to 0, the ray misses the triangle
    if (det < TRI_EPS)
        return false;

    Vec3 tvec = r.origin - m;
    float uu = dot(tvec, pvec) * sign;
    if (uu < 0 || uu > det)
        return false;

    Vec3 qvec = cross(tvec, u);
    float vv = dot(r.dir, qvec) * sign;
    if (vv < 0 || (uu + vv) > det)
        return false;

    float invDet = 1 / det;
    float t = dot(v, qvec) * invDet * sign;

    if (t < tmin || t > tmax)
        return false;

    uu *= invDet;
    vv *= invDet;

    rec->t = t;
    rec->p = r.eval(t);
    rec->n = ((mn * (1 - uu - vv)) + (un * uu) + (vn * vv)).normalize();
    rec->u = uu;
    rec->v = vv;
    rec->mat_ptr = mat_ptr;
    return true;
#else
    // Watertight Ray/Triangle Intersection (http://jcgt.org/published/0002/01/05/paper.pdf)
    /* calculate vertices relative to ray origin */
    const Vec3 a = a - r.origin;
    const Vec3 b = b - r.origin;
    const Vec3 c = c - r.origin;

    float Sx = r.shear.x;
    float Sy = r.shear.y;
    float Sz = r.shear.z;
    size_t kx = r.kx;
    size_t ky = r.ky;
    size_t kz = r.kz;

    /* perform shear and scale of vertices */
    const float ax = a[kx] - Sx * a[kz];
    const float ay = a[ky] - Sy * a[kz];
    const float bx = b[kx] - Sx * b[kz];
    const float by = b[ky] - Sy * b[kz];
    const float cx = c[kx] - Sx * c[kz];
    const float cy = c[ky] - Sy * c[kz];

    /* calculate scaled barycentric coordinates */
    float u = cx * by - cy * bx;
    float v = ax * cy - ay * cx;
    float w = bx * ay - by * ax;

    /* fallback to test against edges using double precision */

    if (u == 0.0f || v == 0.0f || w == 0.0f) {
        double CxBy = (double) cx * (double) by;
        double CyBx = (double) cy * (double) bx;
        u = (float) (CxBy - CyBx);
        double AxCy = (double) ax * (double) cy;
        double AyCx = (double) ay * (double) cx;
        v = (float) (AxCy - AyCx);
        double BxAy = (double) bx * (double) ay;
        double ByAx = (double) by * (double) ax;
        w = (float) (BxAy - ByAx);
    }

    /* Perform edge tests. Moving this test before and at the end of the previous conditional gives higher performance. */
#ifdef BACKFACE_CULLING
    if (u < 0.0f || v < 0.0f || w < 0.0f) return false;
#else
    if ((u < 0.0f || v < 0.0f || w < 0.0f) &&
        (u > 0.0f || v > 0.0f || w > 0.0f)) return false;
#endif

    float det = u + v + w;
    if (det == 0.0f)
        return false;

    /* Calculate scaled z-coordinates of vertices and use them to calculate the hit distance.*/
    const float Az = Sz * a[kz];
    const float Bz = Sz * b[kz];
    const float Cz = Sz * c[kz];
    const float t = u * Az + v * Bz + w * Cz;

#ifdef BACKFACE_CULLING
    if (t < 0.0f || t > tmax * det)
        return false;
#else
    m128 sign { 0x80000000u,0,0,0 };
    m128 det_sign = _mm_and_ps(_mm_set_ss(det), sign);

    float signedt = _mm_cvtss_f32(_mm_xor_ps(_mm_set_ss(t), det_sign));
    float absdet = MRT::abs(det);
    if ((signedt < 0.0f) || (signedt > tmax * absdet))
        return false;
#endif
    /* normalize u, v, w, and t */
    const float rcpDet = 1.0f / det;
    rec->u = u * rcpDet;
    rec->v = v * rcpDet;
    //rec->w = w * rcpDet;
    rec->t = t * rcpDet;
    rec->p = r.eval(t * rcpDet);
    rec->n = ((an * (1 - u - v)) + (bn * u) + (cn * v)).normalize();
    rec->mat_ptr = mat_ptr;
#endif
    return true;
}