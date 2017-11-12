#include "common.h"
#include "vec3.h"
#include <math.h>
#include "mrt_math.h"
#include "mat4.h"


const Mat4 Mat4::Identity = { 1, 0, 0, 0,
                              0, 1, 0, 0,
                              0, 0, 1, 0,
                              0, 0, 0, 1 };

Mat4 Mat4::Invert(const Mat4& m) {
#if 0
    const Vec3 x = m.c0.swizzle<W, W, W, W>().m;
    const Vec3 y = m.c1.swizzle<W, W, W, W>().m;
    const Vec3 z = m.c2.swizzle<W, W, W, W>().m;
    const Vec3 w = m.c3.swizzle<W, W, W, W>().m;

    Vec3 s = cross(m.c0, m.c1);
    Vec3 t = cross(m.c2, m.c3);
    Vec3 u = y * m.c0 - x * m.c1;
    Vec3 v = w * m.c2 - z * m.c3;

    float invDet = 1.0f / (dot(s, v) + dot(t, u));
    s *= invDet;
    t *= invDet;
    u *= invDet;
    v *= invDet;

    Vec3 r0 = cross(m.c1, v) + t * y;
    Vec3 r1 = cross(v, m.c0) - t * x;
    Vec3 r2 = cross(m.c3, u) + s * w;
    Vec3 r3 = cross(u, m.c2) - s * z;

    static constexpr m128 signs { 0x80000000u,0x80000000u,0x80000000u,0x80000000u };

    __m128 r0w = _mm_xor_ps(_mm_dp_ps(m.c1.m, t.m, 0x71), signs); // -dot(m.c1, t)
    __m128 r1w = _mm_dp_ps(m.c0.m, t.m, 0x72);                    //  dot(m.c0, t)
    __m128 r2w = _mm_xor_ps(_mm_dp_ps(m.c3.m, s.m, 0x74), signs); // -dot(m.c3, s)
    __m128 r3w = _mm_dp_ps(m.c2.m, s.m, 0x78);                    //  dot(m.c2, s)

    __m128 r01xxyy = _mm_unpacklo_ps(r0.m, r1.m); // r0x r1x r0y r1y
    __m128 r23xxyy = _mm_unpacklo_ps(r2.m, r3.m); // r2x r3x r2y r3y
    __m128 r01zzww = _mm_unpackhi_ps(r0.m, r1.m); // r0z r1z r0w r1w
    __m128 r23zzww = _mm_unpackhi_ps(r2.m, r3.m); // r2z r3z r2w r3w

    Mat4 res;
    res.c0 = _mm_movelh_ps(r01xxyy, r23xxyy); // r0x r1x r2x r3x
    res.c1 = _mm_movehl_ps(r23xxyy, r01xxyy); // r0y r1y r2y r3y
    res.c2 = _mm_movelh_ps(r01zzww, r23zzww); // r0z r1z r2z r3z

                                              // same cycles/uops, different ports (blend shares with dpps and mul, add shares with dpps and sub (on IVB). HSW can do blend on port 5 as well)
#if 0
    res.c3 = _mm_add_ps(_mm_add_ps(r0w, r1w), _mm_add_ps(r2w, r3w));
#else
    res.c3 = _mm_blend_ps(_mm_blend_ps(r0w, r1w, 0x2), _mm_blend_ps(r2w, r3w, 0x8), 0xC);
#endif

#else
    // TODO: analyze this version and possibly rewrite/comment it to be more legible (maybe AVX-256 version?)

    __m128 f1 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(m.c2.m, m.c1.m, 0xAA),
                                      _mm_permute_ps(_mm_shuffle_ps(m.c3.m, m.c2.m, 0xFF), 0x80)),
                           _mm_mul_ps(_mm_permute_ps(_mm_shuffle_ps(m.c3.m, m.c2.m, 0xAA), 0x80),
                                      _mm_shuffle_ps(m.c2.m, m.c1.m, 0xFF)));

    __m128 f2 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(m.c2.m, m.c1.m, 0x55),
                                      _mm_permute_ps(_mm_shuffle_ps(m.c3.m, m.c2.m, 0xFF), 0x80)),
                           _mm_mul_ps(_mm_permute_ps(_mm_shuffle_ps(m.c3.m, m.c2.m, 0x55), 0x80),
                                      _mm_shuffle_ps(m.c2.m, m.c1.m, 0xFF)));

    __m128 f3 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(m.c2.m, m.c1.m, 0x55),
                                      _mm_permute_ps(_mm_shuffle_ps(m.c3.m, m.c2.m, 0xAA), 0x80)),
                           _mm_mul_ps(_mm_permute_ps(_mm_shuffle_ps(m.c3.m, m.c2.m, 0x55), 0x80),
                                      _mm_shuffle_ps(m.c2.m, m.c1.m, 0xAA)));

    __m128 f4 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(m.c2.m, m.c1.m, 0x00),
                                      _mm_permute_ps(_mm_shuffle_ps(m.c3.m, m.c2.m, 0xFF), 0x80)),
                           _mm_mul_ps(_mm_permute_ps(_mm_shuffle_ps(m.c3.m, m.c2.m, 0x00), 0x80),
                                      _mm_shuffle_ps(m.c2.m, m.c1.m, 0xFF)));

    __m128 f5 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(m.c2.m, m.c1.m, 0x00),
                                      _mm_permute_ps(_mm_shuffle_ps(m.c3.m, m.c2.m, 0xAA), 0x80)),
                           _mm_mul_ps(_mm_permute_ps(_mm_shuffle_ps(m.c3.m, m.c2.m, 0x00), 0x80),
                                      _mm_shuffle_ps(m.c2.m, m.c1.m, 0xAA)));

    __m128 f6 = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(m.c2.m, m.c1.m, 0x00),
                                      _mm_permute_ps(_mm_shuffle_ps(m.c3.m, m.c2.m, 0x55), 0x80)),
                           _mm_mul_ps(_mm_permute_ps(_mm_shuffle_ps(m.c3.m, m.c2.m, 0x00), 0x80),
                                      _mm_shuffle_ps(m.c2.m, m.c1.m, 0x55)));

    __m128 v1 = _mm_permute_ps(_mm_shuffle_ps(m.c1.m, m.c0.m, 0x00), 0xA8);
    __m128 v2 = _mm_permute_ps(_mm_shuffle_ps(m.c1.m, m.c0.m, 0x55), 0xA8);
    __m128 v3 = _mm_permute_ps(_mm_shuffle_ps(m.c1.m, m.c0.m, 0xAA), 0xA8);
    __m128 v4 = _mm_permute_ps(_mm_shuffle_ps(m.c1.m, m.c0.m, 0xFF), 0xA8);
    __m128 s1 = _mm_set_ps(-0.0f, 0.0f, -0.0f, 0.0f);
    __m128 s2 = _mm_set_ps(0.0f, -0.0f, 0.0f, -0.0f);

    __m128 i1 = _mm_xor_ps(s1, _mm_add_ps(_mm_sub_ps(_mm_mul_ps(v2, f1),
                                                     _mm_mul_ps(v3, f2)),
                                          _mm_mul_ps(v4, f3)));

    __m128 i2 = _mm_xor_ps(s2, _mm_add_ps(_mm_sub_ps(_mm_mul_ps(v1, f1),
                                                     _mm_mul_ps(v3, f4)),
                                          _mm_mul_ps(v4, f5)));

    __m128 i3 = _mm_xor_ps(s1, _mm_add_ps(_mm_sub_ps(_mm_mul_ps(v1, f2),
                                                     _mm_mul_ps(v2, f4)),
                                          _mm_mul_ps(v4, f6)));

    __m128 i4 = _mm_xor_ps(s2, _mm_add_ps(_mm_sub_ps(_mm_mul_ps(v1, f3),
                                                     _mm_mul_ps(v2, f5)),
                                          _mm_mul_ps(v3, f6)));

    __m128 d = _mm_mul_ps(m.c0.m, _mm_movelh_ps(_mm_unpacklo_ps(i1, i2),
                                                _mm_unpacklo_ps(i3, i4)));
    d = _mm_add_ps(d, _mm_permute_ps(d, 0x4E));
    d = _mm_add_ps(d, _mm_permute_ps(d, 0x11));
    d = _mm_div_ps(_mm_set1_ps(1.0f), d);
    Mat4 res(_mm_mul_ps(i1, d),
             _mm_mul_ps(i2, d),
             _mm_mul_ps(i3, d),
             _mm_mul_ps(i4, d));
#endif
    return res;
}

Mat4 Mat4::Transpose(const Mat4& m) {
    Mat4 res;
    __m128 tmp3, tmp2, tmp1, tmp0;
#if 1
    tmp0 = _mm_unpacklo_ps(m.c0.m, m.c1.m); // c0x c1x c0y c1y
    tmp2 = _mm_unpacklo_ps(m.c2.m, m.c3.m); // c2x c3x c2y c3y
    tmp1 = _mm_unpackhi_ps(m.c0.m, m.c1.m); // c0z c1z c0w c1w
    tmp3 = _mm_unpackhi_ps(m.c2.m, m.c3.m); // c2z c3z c2w c3w
    res.c0 = _mm_movelh_ps(tmp0, tmp2);     // c0x c1x c2x c3x
    res.c1 = _mm_movehl_ps(tmp2, tmp0);     // c0y c1y c2y c3y
    res.c2 = _mm_movelh_ps(tmp1, tmp3);     // c0z c1z c2z c3z
    res.c3 = _mm_movehl_ps(tmp3, tmp1);     // c0w c1w c2w c3w
#else
    // TODO: benchmark
    tmp0 = _mm_shuffle_ps(m.c0.m, m.c1.m, 0x44);
    tmp2 = _mm_shuffle_ps(m.c0.m, m.c1.m, 0xEE);
    tmp1 = _mm_shuffle_ps(m.c2.m, m.c3.m, 0x44);
    tmp3 = _mm_shuffle_ps(m.c2.m, m.c3.m, 0xEE);

    res.c0 = _mm_shuffle_ps(tmp0, tmp1, 0x88);
    res.c1 = _mm_shuffle_ps(tmp0, tmp1, 0xDD);
    res.c2 = _mm_shuffle_ps(tmp2, tmp3, 0x88);
    res.c3 = _mm_shuffle_ps(tmp2, tmp3, 0xDD);
#endif
    return res;
}


Mat4 Mat4::Translate(const Vec3& t) {
    return Mat4(1, 0, 0, t.x,
                0, 1, 0, t.y,
                0, 0, 1, t.z,
                0, 0, 0, 1);
}

Mat4 Mat4::Scale(float s) {
    return Mat4(s, 0, 0, 0,
                0, s, 0, 0,
                0, 0, s, 0,
                0, 0, 0, 1);
}

Mat4 Mat4::Scale(float sx, float sy, float sz) {
    return Mat4(sx, 0, 0, 0,
                0, sy, 0, 0,
                0, 0, sz, 0,
                0, 0, 0, 1);
}

// scale along axis a
Mat4 Mat4::Scale(float s, const Vec3& a) {
    Mat4 res;
    s -= 1.0f;
    Vec3 as = a * s;
    Vec3 a2 = a * as + Vec3(1, 1, 1);
    Vec3 m = as * a.swizzle<Y, Z, X>(); // [x*y, y*z, z*x]
    res.c0 = _mm_or_ps(a2.swizzle<X, W, W, W>().m, m.swizzle<W, X, Y, W>().m);
    res.c1 = _mm_or_ps(a2.swizzle<W, Y, W, W>().m, m.swizzle<X, W, Y, W>().m);
    res.c2 = _mm_or_ps(a2.swizzle<W, W, Z, W>().m, m.swizzle<Z, Y, W, W>().m);
    res.c3 = Vec4(0, 0, 0, 1);
    return res;
}

// reflect through plane perpendicular to vector n
Mat4 Mat4::Reflect(const Vec3& n) {
    return Mat4::Scale(-1.0f, n);
}

// reflect through vector a
Mat4 Mat4::Involution(const Vec3& a) {
    Mat4 res;
    Vec3 as = a * 2.0f;
    Vec3 a2 = a * as - Vec3(1, 1, 1); // the only difference to a scaling matrix is the - here
    Vec3 m = as * a.swizzle<Y, Z, X>(); // [x*y, y*z, z*x]
    res.c0 = _mm_or_ps(a2.swizzle<X, W, W, W>().m, m.swizzle<W, X, Y, W>().m);
    res.c1 = _mm_or_ps(a2.swizzle<W, Y, W, W>().m, m.swizzle<X, W, Y, W>().m);
    res.c2 = _mm_or_ps(a2.swizzle<W, W, Z, W>().m, m.swizzle<Z, Y, W, W>().m);
    res.c3 = Vec4(0, 0, 0, 1);
    return res;
}

// TODO: check if we can SIMDify this a bit, seems difficult.
Mat4 Mat4::Rotate(const Vec3& axis, float radians) {
    float s = sinf(radians);
    float c = cosf(radians);
    float d = 1.0f - c;
    float x = axis.x * d;
    float y = axis.y * d;
    float z = axis.z * d;
    float axay = x * axis.y;
    float axaz = x * axis.z;
    float ayaz = y * axis.z;

    return Mat4(c + x * axis.x, axay - s * axis.z, axaz + s * axis.y, 0,
                axay + s * axis.z, c + y * axis.y, ayaz - s * axis.x, 0,
                axaz - s * axis.y, ayaz + s * axis.x, c + z * axis.z, 0,
                0, 0, 0, 1);
}

Mat4 Mat4::RotateX(float radians) {
    float s = sinf(radians);
    float c = cosf(radians);
    return Mat4(1, 0, 0, 0,
                0, c, -s, 0,
                0, s, c, 0,
                0, 0, 0, 1);
}

Mat4 Mat4::RotateY(float radians) {
    float s = sinf(radians);
    float c = cosf(radians);
    return Mat4(c, 0, s, 0,
                0, 1, 0, 0,
                -s, 0, c, 0,
                0, 0, 0, 1);
}

Mat4 Mat4::RotateZ(float radians) {
    float s = sinf(radians);
    float c = cosf(radians);
    return Mat4(c, -s, 0, 0,
                s, c, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1);
}
