#pragma once
#include <math.h>
#include <stdint.h>
#include "mrt_math.h"

#define MRT_GAMMA 2.2f

// NOTE: overall, MSVC likes vbroadcastss instructions instead of vshufps or vpermilps, _mm_set_ps1 mitigates this problem somewhat

// indices for swizzling
enum sw_idx { X, Y, Z, W };

class alignas(16) vec3 {
public:
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

    vec3() = default;

    vec3(float x, float y, float z) : 
        x(x), y(y), z(z), w(0) {}

    vec3(float f) : 
        x(f), y(f), z(f), w(0) {}

    vec3(__m128 m) : m(m) {}

    inline float operator[](size_t i) const {
        return e[i];
    }

    inline float& operator[](size_t i) {
        return e[i];
    }

    inline float length() const;
    inline float sdot() const;
    inline vec3& normalize();
    inline vec3& gamma_correct();
    inline vec3& inv_gamma_correct();

    // NOTE: swizzling with W can be used to zero out a component
    template <sw_idx x, sw_idx y, sw_idx z>
    inline vec3 swizzle() {
        return _mm_permute_ps(m, _MM_SHUFFLE(3, z, y, x));
    }

    template <sw_idx x, sw_idx y, sw_idx z, sw_idx w>
    inline vec3 swizzle() {
        return _mm_permute_ps(m, _MM_SHUFFLE(w, z, y, x));
    }
};

inline vec3 operator-(const vec3& v) {
    static constexpr m128 signs { 0x80000000u,0x80000000u,0x80000000u,0x80000000u };
    return _mm_xor_ps(v.m, signs.f128);
}

inline vec3 operator+(const vec3& v1, const vec3& v2) {
    return _mm_add_ps(v1.m, v2.m);
}
inline vec3 operator-(const vec3& v1, const vec3& v2) {
    return _mm_sub_ps(v1.m, v2.m);
}

inline vec3 operator*(const vec3& v, const float f) {
    __m128 m = _mm_set_ps1(f);
    return _mm_mul_ps(v.m, m);
}
inline vec3 operator*(const float f, const vec3& v) {
    __m128 m = _mm_set_ps1(f);
    return _mm_mul_ps(v.m, m);
}

inline vec3 operator/(const vec3& v, const float f) {
    return _mm_div_ps(v.m, _mm_set_ps1(f));
}

inline vec3& operator+=(vec3 &v1, const vec3& v2) {
    v1 = v1 + v2;
    return v1;
}
inline vec3& operator-=(vec3 &v1, const vec3& v2) {
    v1 = v1 - v2;
    return v1;
}

inline vec3& operator*=(vec3 &v1, const float f) { 
    v1 = v1 * f;
    return v1;
}
inline vec3& operator/=(vec3 &v1, const float f) {
    v1 = v1 / f;
    return v1;
}

// element wise mul and div, mostly for colors

inline vec3 operator*(const vec3& v1, const vec3& v2) {
    return _mm_mul_ps(v1.m, v2.m);
}
inline vec3 operator/(const vec3& v1, const vec3& v2) {
    return _mm_div_ps(v1.m, v2.m);
}
inline vec3& operator*=(vec3 &v1, const vec3& v2) {
    v1 = v1 * v2;
    return v1;
}
inline vec3& operator/=(vec3 &v1, const vec3& v2) {
    v1 = v1 / v2;
    return v1;
}

////

inline float dot(const vec3& v1, const vec3& v2) {
    return _mm_cvtss_f32(_mm_dp_ps(v1.m, v2.m, 0x71));
}

// NOTE: intrinsics seem to be worse on micro-benchmarks, but much better in a real run
inline vec3 cross(const vec3& v1, const vec3& v2) {
#if 1
    static constexpr int maskYZX = _MM_SHUFFLE(3, 0, 2, 1); // NOTE: read _MM_SHUFFLE from right to left
    static constexpr int maskZXY = _MM_SHUFFLE(3, 1, 0, 2);

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
    return vec3(v1.y * v2.z - v1.z * v2.y,
                v1.z * v2.x - v1.x * v2.z,
                v1.x * v2.y - v1.y * v2.x);
#endif
}

inline float sdot(const vec3& v) {
    return dot(v, v);
}

inline float vec3::sdot() const {
    return dot(*this, *this);
}

inline float vec3::length() const {
    float f = sdot();
    return mrt_sqrt(f);
}

inline vec3 normalize(const vec3& v) {
    return v / v.length();
}

inline vec3& vec3::normalize() {
    *this /= this->length();
    return *this;
}

inline vec3& vec3::gamma_correct() {
    r = pow(r, 1 / MRT_GAMMA);
    g = pow(g, 1 / MRT_GAMMA);
    b = pow(b, 1 / MRT_GAMMA);
    return *this;
}

inline vec3 gamma_correct(const vec3& c) {
    return vec3(pow(c.r, 1 / MRT_GAMMA), pow(c.g, 1 / MRT_GAMMA), pow(c.b, 1 / MRT_GAMMA));
}

inline vec3& vec3::inv_gamma_correct() {
    r = pow(r, MRT_GAMMA);
    g = pow(g, MRT_GAMMA);
    b = pow(b, MRT_GAMMA);
    return *this;
}

inline vec3 inv_gamma_correct(const vec3& c) {
    return vec3(pow(c.r, MRT_GAMMA), pow(c.g, MRT_GAMMA), pow(c.b, MRT_GAMMA));
}

inline vec3 vmin(const vec3& a, const vec3& b) {
    return _mm_min_ps(a.m, b.m);
}

inline vec3 vmin(const vec3& a, const vec3& b, const vec3& c) {
    return vmin(vmin(a, b), c);
}

inline vec3 vmax(const vec3& a, const vec3& b) {
    return _mm_max_ps(a.m, b.m);
}

inline vec3 vmax(const vec3& a, const vec3& b, const vec3& c) {
    return vmax(vmax(a, b), c);
}

inline vec3 reflect(const vec3& v, const vec3& n) {
    float dp = 2 * dot(v, n);
    vec3 a = _mm_set_ps1(dp);
    return v - (a * n);
}


// refracted vector not normalized!
inline bool refract(const vec3& v, const vec3& n, float ni_over_nt, vec3* refracted) {
    float ncosI = dot(v, n);
    float sinT2 = (ni_over_nt * ni_over_nt) * (1.0f - ncosI*ncosI);

    if (sinT2 <= 1.0f) {
        float cosT = mrt_sqrt(1.0f - sinT2);
        float cosI = -ncosI;
        *refracted = _mm_set_ps1(ni_over_nt) * v + _mm_set_ps1(ni_over_nt * cosI - cosT) * n;
        return true;
    }
    else { // total inner reflection
        return false;
    }
}