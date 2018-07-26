#pragma once
#include "common.h"
#include <algorithm>

#if _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#define M_PI_F 3.14159265358979323846f
#define RAD(a) ((a) * (M_PI_F / 180.0f))
#define DEG(r) ((r) * (180.0f / M_PI_F))

union alignas(16) m128 {
    uint8_t  u8[16];
    uint16_t u16[8];
    uint32_t u32[4];
    uint64_t u64[2];
    float    f32[4];
    double   f64[2];
    __m128   f128;
    __m128d  d128;
    __m128i  i128;

    constexpr m128(uint8_t a, uint8_t b, uint8_t c, uint8_t d,
                   uint8_t e, uint8_t f, uint8_t g, uint8_t h,
                   uint8_t i, uint8_t j, uint8_t k, uint8_t l,
                   uint8_t m, uint8_t n, uint8_t o, uint8_t p) : u8{ a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p } {}

    constexpr m128(uint16_t a, uint16_t b, uint16_t c, uint16_t d,
                   uint16_t e, uint16_t f, uint16_t g, uint16_t h) : u16{ a,b,c,d,e,f,g,h } {}

    constexpr m128(uint32_t a, uint32_t b, uint32_t c, uint32_t d) : u32{ a,b,c,d } {}
    constexpr m128(uint64_t a, uint64_t b) : u64{ a,b } {}
    constexpr m128(float a, float b, float c, float d) : f32{ a,b,c,d } {}
    constexpr m128(double a, double b) : f64{ a,b } {}
    constexpr m128(__m128  m) : f128(m) {}
    constexpr m128(__m128d m) : d128(m) {}
    constexpr m128(__m128i m) : i128(m) {}

    inline operator __m128 () const { return f128; }
    inline operator __m128i() const { return i128; }
    inline operator __m128d() const { return d128; }
};

namespace MRT {

    inline uint32_t nextPo2(uint32_t v) {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v++;
        return v;
    }

    inline uint32 lzcnt(uint32 v) {
#if _MSC_VER
        unsigned long i;
        _BitScanReverse(&i, v);
        i = 31 - i;
#else
#error INSERT LZCNT INTRINSIC HERE
#endif
        return i;
    }

    inline uint32 log2U32(uint32 v) {
        if (v == 0) return 0;
        else        return 31 - lzcnt(v);
    }

    inline float sqrt(float f) {
        __m128 m = _mm_set_ss(f);
        return _mm_cvtss_f32(_mm_sqrt_ss(m));
    }

    inline float abs(float f) {
        static constexpr m128 signs { 0x7FFFFFFFu,0x7FFFFFFFu,0x7FFFFFFFu,0x7FFFFFFFu };
        __m128 m = _mm_set_ss(f);
        return _mm_cvtss_f32(_mm_and_ps(m, signs));
    }

    template <typename T>
    inline T clamp(T v, T min, T max) {
        return std::max<T>(std::min<T>(v, max), min);
    }
}