#pragma once

#include "vec3.h"
#include "mrt_math.h"

// TODO: finish Mat4 type, make more efficient structs for transforms.

// column-major 4x4 matrix (aligned at 32 bytes to allow AVX-256 operations)
struct alignas(32) Mat4 {
    union {
        struct {
            Vec4 c0, c1, c2, c3;
        };
        Vec4 c[4];
        struct {
            float m00, m10, m20, m30;
            float m01, m11, m21, m31;
            float m02, m12, m22, m32;
            float m03, m13, m23, m33;
        };
    };

    static const Mat4 Identity;

    Mat4() = default;

    Mat4(const Vec4 &c0, const Vec4 &c1, const Vec4 &c2, const Vec4 &c3) : c0(c0), c1(c1), c2(c2), c3(c3) {}

    Mat4(const __m128 &c0, const __m128 &c1, const __m128 &c2, const __m128 &c3) : c0(c0), c1(c1), c2(c2), c3(c3) {}

    Mat4(float m00, float m01, float m02, float m03,
         float m10, float m11, float m12, float m13,
         float m20, float m21, float m22, float m23,
         float m30, float m31, float m32, float m33) :
        m00(m00), m10(m10), m20(m20), m30(m30),
        m01(m01), m11(m11), m21(m21), m31(m31),
        m02(m02), m12(m12), m22(m22), m32(m32),
        m03(m03), m13(m13), m23(m23), m33(m33) {}

    const Vec4 &operator[](size_t i) const {
        return c[i];
    }
    
    Vec3 operator*(const Vec3 &v) const {
        __m128 vx = v.swizzle<X, X, X>().m;
        __m128 vy = v.swizzle<Y, Y, Y>().m;
        __m128 vz = v.swizzle<Z, Z, Z>().m;
        __m128 c0 = _mm_mul_ps(vx, c[0].m);
        __m128 c1 = _mm_mul_ps(vy, c[1].m);
        __m128 c2 = _mm_mul_ps(vz, c[2].m);
        __m128 res = _mm_add_ps(_mm_add_ps(c0, c1), c2);
        return res;
    }

    Vec4 operator*(const Vec4 &v) const {
        __m128 vx = v.swizzle<X, X, X, X>().m;
        __m128 vy = v.swizzle<Y, Y, Y, Y>().m;
        __m128 vz = v.swizzle<Z, Z, Z, Z>().m;
        __m128 vw = v.swizzle<W, W, W, W>().m;
        __m128 c0 = _mm_mul_ps(vx, c[0].m);
        __m128 c1 = _mm_mul_ps(vy, c[1].m);
        __m128 c2 = _mm_mul_ps(vz, c[2].m);
        __m128 c3 = _mm_mul_ps(vw, c[3].m); 
        __m128 res = _mm_add_ps(_mm_add_ps(c0, c1), _mm_add_ps(c2, c3));
        return res;
    }

    // considers v as a row vector
    friend Vec4 operator*(const Vec4 &v, const Mat4& m) {
#if 0
        __m128 x = _mm_dp_ps(v.m, m.c0.m, 0xF1);
        __m128 y = _mm_dp_ps(v.m, m.c1.m, 0xF2);
        __m128 z = _mm_dp_ps(v.m, m.c2.m, 0xF4);
        __m128 w = _mm_dp_ps(v.m, m.c3.m, 0xF8);

        return _mm_blend_ps(_mm_blend_ps(x, y, 0x2), _mm_blend_ps(z, w, 0x8), 0xC);
#else
        __m128 m0 = _mm_mul_ps(v.m, m[0].m);
        __m128 m1 = _mm_mul_ps(v.m, m[1].m);
        __m128 m2 = _mm_mul_ps(v.m, m[2].m);
        __m128 m3 = _mm_mul_ps(v.m, m[3].m);

        __m128 xxyy = _mm_unpacklo_ps(m0, m1); // m0x m1x m0y m1y
        __m128 zzww = _mm_unpackhi_ps(m0, m1); // m0z m1z m0w m1w
        __m128 a01  = _mm_add_ps(xxyy, zzww);  //  x + z | y + w

               xxyy = _mm_unpacklo_ps(m2, m3); // m2x m3x m2y m3y
               zzww = _mm_unpackhi_ps(m2, m3); // m2z m3z m2w m3w
        __m128 a23  = _mm_add_ps(xxyy, zzww);  //  x + z | y + w

        __m128 axz = _mm_movelh_ps(a01, a23);  // m01 x+z | m23 x+z
        __m128 ayw = _mm_movehl_ps(a23, a01);  // m01 y+w | m23 y+w
        __m128 res = _mm_add_ps(axz, ayw);     // x+y+z+w | x+y+z+w

        return res;
#endif
    }

    // considers v as a row vector
    friend Vec3 operator*(const Vec3 &v, const Mat4& m) {
        return Vec4(v) * m;
    }

    Mat4 operator*(const Mat4 &m) const {
#if 0
        // intuitive version

        // transpose the left matrix to get rows
        __m128 tmp3, tmp2, tmp1, tmp0;
        __m128 row3, row2, row1, row0;
        tmp0 = _mm_unpacklo_ps(c0.m, c1.m);
        tmp2 = _mm_unpacklo_ps(c2.m, c3.m);
        tmp1 = _mm_unpackhi_ps(c0.m, c1.m);
        tmp3 = _mm_unpackhi_ps(c2.m, c3.m);
        row0 = _mm_movelh_ps(tmp0, tmp2);
        row1 = _mm_movehl_ps(tmp2, tmp0);
        row2 = _mm_movelh_ps(tmp1, tmp3);
        row3 = _mm_movehl_ps(tmp3, tmp1);

        // m_ij = dot product between row i and column j
        float m00 = dot(row0, m.c0);
        float m01 = dot(row0, m.c1);
        float m02 = dot(row0, m.c2);
        float m03 = dot(row0, m.c3);

        float m10 = dot(row1, m.c0);
        float m11 = dot(row1, m.c1);
        float m12 = dot(row1, m.c2);
        float m13 = dot(row1, m.c3);

        float m20 = dot(row2, m.c0);
        float m21 = dot(row2, m.c1);
        float m22 = dot(row2, m.c2);
        float m23 = dot(row2, m.c3);

        float m30 = dot(row3, m.c0);
        float m31 = dot(row3, m.c1);
        float m32 = dot(row3, m.c2);
        float m33 = dot(row3, m.c3);

        Mat4 res(m00, m01, m02, m03,
                 m10, m11, m12, m13,
                 m20, m21, m22, m23,
                 m30, m31, m32, m33);
#elif 0
        // optimized AVX-128 version
        Mat4 res;
        res.c0 = *this * m.c0;
        res.c1 = *this * m.c1;
        res.c2 = *this * m.c2;
        res.c3 = *this * m.c3;
#else
        // optimized AVX-256 version
        Mat4 res;
        __m256 v0, v1, v2, v3;
        __m256 xx, yy, zz, ww;
        __m256 c0c0, c1c1, c2c2, c3c3;

        // _mm256_set_m128() compiles to movaps + vinsertf128, which may (depending on uarch) execute on different ports than vpermilps later
        // broadcast has less instruction bytes, but the same number of uops
#if 0
        c0c0 = _mm256_broadcast_ps(&c0.m);
        c1c1 = _mm256_broadcast_ps(&c1.m);
        c2c2 = _mm256_broadcast_ps(&c2.m);
        c3c3 = _mm256_broadcast_ps(&c3.m);
#else  
        c0c0 = _mm256_set_m128(c0.m, c0.m);
        c1c1 = _mm256_set_m128(c1.m, c1.m);
        c2c2 = _mm256_set_m128(c2.m, c2.m);
        c3c3 = _mm256_set_m128(c3.m, c3.m);
#endif

        // blow out the right matrix's elements to all lanes
        // multiply with left columns and add all results to get the result column
        __m256 c0c1 = _mm256_load_ps(&m.c0.x);
        xx = _mm256_permute_ps(c0c1, _MM_SHUFFLE(X, X, X, X));
        yy = _mm256_permute_ps(c0c1, _MM_SHUFFLE(Y, Y, Y, Y));
        zz = _mm256_permute_ps(c0c1, _MM_SHUFFLE(Z, Z, Z, Z));
        ww = _mm256_permute_ps(c0c1, _MM_SHUFFLE(W, W, W, W));
        v0 = _mm256_mul_ps(xx, c0c0);
        v1 = _mm256_mul_ps(yy, c1c1);
        v2 = _mm256_mul_ps(zz, c2c2);
        v3 = _mm256_mul_ps(ww, c3c3);
        __m256 res_c0c1 = _mm256_add_ps(_mm256_add_ps(v0, v1), _mm256_add_ps(v2, v3));
        _mm256_store_ps(&res.c0.x, res_c0c1);

        __m256 c2c3 = _mm256_load_ps(&m.c2.x);
        xx = _mm256_permute_ps(c2c3, _MM_SHUFFLE(X, X, X, X));
        yy = _mm256_permute_ps(c2c3, _MM_SHUFFLE(Y, Y, Y, Y));
        zz = _mm256_permute_ps(c2c3, _MM_SHUFFLE(Z, Z, Z, Z));
        ww = _mm256_permute_ps(c2c3, _MM_SHUFFLE(W, W, W, W));
        v0 = _mm256_mul_ps(xx, c0c0);
        v1 = _mm256_mul_ps(yy, c1c1);
        v2 = _mm256_mul_ps(zz, c2c2);
        v3 = _mm256_mul_ps(ww, c3c3);
        __m256 res_c2c3 = _mm256_add_ps(_mm256_add_ps(v0, v1), _mm256_add_ps(v2, v3));
        _mm256_store_ps(&res.c2.x, res_c2c3);

#endif
        return res;
    }

    Mat4 operator+(const Mat4 &m) const {
        Mat4 res;
        _mm256_store_ps(&res.c0.x, _mm256_add_ps(_mm256_load_ps(&m.c0.x), _mm256_load_ps(&c0.x)));
        _mm256_store_ps(&res.c2.x, _mm256_add_ps(_mm256_load_ps(&m.c2.x), _mm256_load_ps(&c2.x)));
        return res;
    }

    Mat4 operator-(const Mat4 &m) const {
        Mat4 res;
        _mm256_store_ps(&res.c0.x, _mm256_sub_ps(_mm256_load_ps(&m.c0.x), _mm256_load_ps(&c0.x)));
        _mm256_store_ps(&res.c2.x, _mm256_sub_ps(_mm256_load_ps(&m.c2.x), _mm256_load_ps(&c2.x)));
        return res;
    }

    Mat4 operator-() const {
        Mat4 res;
        _mm256_store_ps(&res.c0.x, _mm256_sub_ps(_mm256_setzero_ps(), _mm256_load_ps(&c0.x)));
        _mm256_store_ps(&res.c2.x, _mm256_sub_ps(_mm256_setzero_ps(), _mm256_load_ps(&c2.x)));
        return res;
    }

    static Mat4 Invert(const Mat4& m);

    static Mat4 Transpose(const Mat4& m);

    static Mat4 Translate(const Vec3& t);

    static Mat4 Scale(float s);
    static Mat4 Scale(float sx, float sy, float sz);
    static Mat4 Scale(float s, const Vec3& a); // scale along axis a

    // reflect through plane perpendicular to vector n
    static Mat4 Reflect(const Vec3& n);

    // reflect through vector a
    static Mat4 Involution(const Vec3& a);

    static Mat4 Rotate(const Vec3& axis, float radians);
    static Mat4 RotateX(float radians);
    static Mat4 RotateY(float radians);
    static Mat4 RotateZ(float radians);
};

