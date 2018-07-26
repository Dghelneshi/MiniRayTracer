#pragma once
#include <math.h>
#include "mrt_math.h"
#include "common.h"

#define MRT_GAMMA 2.2f

// indices for swizzling
enum sw_idx { X, Y, Z, W };

struct alignas(16) Vec4 {
    union {
        float e[4];
        struct {
            float x;
            float y;
            float z;
            float w;
        };
        struct {
            float r;
            float g;
            float b;
            float a;
        };
        __m128 m;
    };

    Vec4() = default;

    Vec4(float x, float y, float z, float w) : 
        x(x), y(y), z(z), w(w) {}

    Vec4(float f) : 
        x(f), y(f), z(f), w(f) {}

    Vec4(const __m128& m) : m(m) {}

    inline float operator[](size_t i) const {
        return e[i];
    }

    inline float& operator[](size_t i) {
        return e[i];
    }

    inline float length() const;
    inline float sdot() const;
    inline Vec4& normalize();
    inline Vec4& normalize_fast();

    template <sw_idx x, sw_idx y, sw_idx z, sw_idx w>
    inline Vec4 swizzle() const {
        return _mm_permute_ps(m, _MM_SHUFFLE(w, z, y, x));
    }
};

inline Vec4 operator-(const Vec4& v) {
    return _mm_xor_ps(v.m, _mm_set1_ps(-0.0f));
}

inline Vec4 operator+(const Vec4& v1, const Vec4& v2) {
    return _mm_add_ps(v1.m, v2.m);
}
inline Vec4 operator-(const Vec4& v1, const Vec4& v2) {
    return _mm_sub_ps(v1.m, v2.m);
}

inline Vec4 operator*(const Vec4& v, const float f) {
    __m128 m = _mm_set_ps1(f);
    return _mm_mul_ps(v.m, m);
}
inline Vec4 operator*(const float f, const Vec4& v) {
    __m128 m = _mm_set_ps1(f);
    return _mm_mul_ps(v.m, m);
}

inline Vec4 operator/(const Vec4& v, const float f) {
    return _mm_div_ps(v.m, _mm_set_ps1(f));
}

inline Vec4& operator+=(Vec4 &v1, const Vec4& v2) {
    v1 = v1 + v2;
    return v1;
}
inline Vec4& operator-=(Vec4 &v1, const Vec4& v2) {
    v1 = v1 - v2;
    return v1;
}

inline Vec4& operator*=(Vec4 &v1, const float f) { 
    v1 = v1 * f;
    return v1;
}
inline Vec4& operator/=(Vec4 &v1, const float f) {
    v1 = v1 / f;
    return v1;
}

inline Vec4 operator*(const Vec4& v1, const Vec4& v2) {
    return _mm_mul_ps(v1.m, v2.m);
}
inline Vec4 operator/(const Vec4& v1, const Vec4& v2) {
    return _mm_div_ps(v1.m, v2.m);
}
inline Vec4& operator*=(Vec4 &v1, const Vec4& v2) {
    v1 = v1 * v2;
    return v1;
}
inline Vec4& operator/=(Vec4 &v1, const Vec4& v2) {
    v1 = v1 / v2;
    return v1;
}

// see also dot(Vec3,Vec3)
inline float dot(const Vec4& v1, const Vec4& v2) {
    __m128 xyz = _mm_mul_ps(v1.m, v2.m);
    __m128 y = _mm_permute_ps(xyz, _MM_SHUFFLE(W, W, W, Y));
    __m128 z = _mm_permute_ps(xyz, _MM_SHUFFLE(W, W, W, Z));
    __m128 w = _mm_permute_ps(xyz, _MM_SHUFFLE(W, W, W, W));
    return _mm_cvtss_f32(_mm_add_ss(_mm_add_ss(xyz, y), _mm_add_ss(z, w)));
}

inline float sdot(const Vec4& v) {
    return dot(v, v);
}

inline float Vec4::sdot() const {
    return dot(*this, *this);
}

inline float Vec4::length() const {
    float f = sdot();
    return MRT::sqrt(f);
}

inline Vec4 normalize(const Vec4& v) {
    return v / v.length();
}

inline Vec4& Vec4::normalize() {
    *this /= this->length();
    return *this;
}

inline Vec4 normalize_fast(const Vec4& v) {
    float f = sdot(v);
    return v * _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(f)));
}

inline Vec4& Vec4::normalize_fast() {
    float f = sdot();
    *this *= _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(f)));
    return *this;
}

inline Vec4 vmin(const Vec4& a, const Vec4& b) {
    return _mm_min_ps(a.m, b.m);
}

inline Vec4 vmin(const Vec4& a, const Vec4& b, const Vec4& c) {
    return vmin(vmin(a, b), c);
}

inline Vec4 vmax(const Vec4& a, const Vec4& b) {
    return _mm_max_ps(a.m, b.m);
}

inline Vec4 vmax(const Vec4& a, const Vec4& b, const Vec4& c) {
    return vmax(vmax(a, b), c);
}

inline Vec4 vabs(const Vec4& a) {
    static constexpr m128 signs { 0x7FFFFFFFu, 0x7FFFFFFFu, 0x7FFFFFFFu, 0x7FFFFFFFu };
    return _mm_and_ps(a.m, signs);
}

inline Vec4 reflect(const Vec4& v, const Vec4& n) {
    float dp = 2 * dot(v, n);
    Vec4 a = _mm_set_ps1(dp);
    return v - (a * n);
}


// refracted vector not normalized!
inline bool refract(const Vec4& v, const Vec4& n, float ni_over_nt, Vec4* refracted) {
    float ncosI = dot(v, n);
    float sinT2 = (ni_over_nt * ni_over_nt) * (1.0f - ncosI*ncosI);

    if (sinT2 <= 1.0f) {
        float cosT = MRT::sqrt(1.0f - sinT2);
        float cosI = -ncosI;
        *refracted = _mm_set_ps1(ni_over_nt) * v + _mm_set_ps1(ni_over_nt * cosI - cosT) * n;
        return true;
    }
    else { // total inner reflection
        return false;
    }
}


struct Vec3 final : public Vec4 {
    Vec3() = default;
    Vec3(float x, float y, float z) : Vec4(x,y,z,0) {}
    explicit Vec3(float f) : Vec3(f, f, f) {}
    Vec3(const __m128& m) : Vec4(m) {}
    Vec3(const Vec4& v) : Vec4(v.m) {}

    // NOTE: swizzling with W can be used to zero out a component
    template <sw_idx x, sw_idx y, sw_idx z>
    inline Vec3 swizzle() const {
        return _mm_permute_ps(m, _MM_SHUFFLE(W, z, y, x));
    }
    template <sw_idx x, sw_idx y, sw_idx z, sw_idx w>
    inline Vec3 swizzle() const {
        return _mm_permute_ps(m, _MM_SHUFFLE(w, z, y, x));
    }

    inline Vec3& gamma_correct();
    inline Vec3& inv_gamma_correct();

};

inline Vec3 operator/(const Vec3& v1, const Vec3& v2) {
    static constexpr m128 maskXYZ { 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0x00000000u };
    __m128 m = _mm_div_ps(v1.m, v2.m);
    return _mm_and_ps(m, maskXYZ);
}

inline Vec3& operator/=(Vec3 &v1, const Vec3& v2) {
    v1 = v1 / v2;
    return v1;
}

inline float dot(const Vec3& v1, const Vec3& v2) {
#if 0
    // bad on AMD due to inefficient hardware implementation, benchmarks slower on Intel too for unknown reasons (does not pipeline well?)
    return _mm_cvtss_f32(_mm_dp_ps(v1.m, v2.m, 0x71));
#elif 0
    // potentially bad on AMD due to separated integer and float pipelines
    m128 xyz = _mm_mul_ps(v1.m, v2.m);
    m128 y = _mm_srli_si128(xyz, 4);
    m128 z = _mm_srli_si128(xyz, 8);
    return _mm_cvtss_f32(_mm_add_ss(_mm_add_ss(xyz, y), z));
#else
    __m128 xyz = _mm_mul_ps(v1.m, v2.m);
    __m128 y = _mm_permute_ps(xyz, _MM_SHUFFLE(W, W, W, Y));
    __m128 z = _mm_permute_ps(xyz, _MM_SHUFFLE(W, W, W, Z));
    return _mm_cvtss_f32(_mm_add_ss(_mm_add_ss(xyz, y), z));
#endif
}

inline Vec3 cross(const Vec3& v1, const Vec3& v2) {
#if 1
    static constexpr int maskYZX = _MM_SHUFFLE(W, X, Z, Y);
    static constexpr int maskZXY = _MM_SHUFFLE(W, Y, X, Z);

    return _mm_sub_ps(
        _mm_mul_ps(
            _mm_permute_ps(v1.m, maskYZX),
            _mm_permute_ps(v2.m, maskZXY)
        ),
        _mm_mul_ps(
            _mm_permute_ps(v1.m, maskZXY),
            _mm_permute_ps(v2.m, maskYZX)
        )
    );
#else
    return Vec3(v1.y * v2.z - v1.z * v2.y,
                v1.z * v2.x - v1.x * v2.z,
                v1.x * v2.y - v1.y * v2.x);
#endif
}

// relative luminance BT.709
inline float luminance(const Vec3& v) {
    const Vec3 c(0.212655f, 0.715158f, 0.072187f);
    Vec3 r = (v * c);
    return r.r + r.g + r.b;
}

inline Vec3 log(const Vec3& v) {
    return Vec3(logf(v.x), logf(v.y), logf(v.z));
}

inline Vec3 exp(const Vec3& v) {
    return Vec3(expf(v.x), expf(v.y), expf(v.z));
}

inline Vec3 log10(const Vec3& v) {
    return Vec3(log10f(v.x), log10f(v.y), log10f(v.z));
}

inline Vec3 pow(const Vec3& v, float f) {
    return Vec3(pow(v.x, f), pow(v.y, f), pow(v.z, f));
}

inline Vec3& Vec3::gamma_correct() {
    *this = pow(*this, (1.0f / MRT_GAMMA));
    return *this;
}

inline Vec3 gamma_correct(const Vec3& c) {
    return pow(c, (1.0f / MRT_GAMMA));
}

inline Vec3& Vec3::inv_gamma_correct() {
    *this = pow(*this, MRT_GAMMA);
    return *this;
}

inline Vec3 inv_gamma_correct(const Vec3& c) {
    return pow(c, MRT_GAMMA);
}


inline size_t max_dim(const Vec3& a) {
    float v0 = a.x;
    float v1 = a.y;
    float v2 = a.z;
    bool v01 = (v0 > v1);
    bool v02 = (v0 > v2);
    bool v12 = (v1 > v2);
    return v01 ? (v02 ? 0 : 2) : (v12 ? 1 : 2);
}

// converts Vec3 color to 32-bit ARGB (memory order BGRA)
inline uint32 ARGB32(const Vec3& v) {
    Vec3 color = vmin(v, Vec3(1.0f)) * 255.99f;
    uint32 red   = (uint32) (color.r);
    uint32 green = (uint32) (color.g);
    uint32 blue  = (uint32) (color.b);
    return (red << 16) | (green << 8) | blue;
}