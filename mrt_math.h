#pragma once
#include <stdint.h>

#if _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#define M_PI_F 3.14159265358979323846f

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
    constexpr m128(__m128 m) : f128(m) {}
    constexpr m128(__m128d m) : d128(m) {}
    constexpr m128(__m128i m) : i128(m) {}
};

inline float mrt_sqrt(float f) {
    // MSVC sometimes wastes an extra register when you use either load or set, is somehow dependent on context
    __m128 m = _mm_load_ss(&f);
    return _mm_cvtss_f32(_mm_sqrt_ss(m));
}

inline float mrt_abs(float f) {
    static constexpr m128 signs { 0x7FFFFFFFu,0x7FFFFFFFu,0x7FFFFFFFu,0x7FFFFFFFu };
    __m128 m = _mm_set_ss(f);
    return _mm_cvtss_f32(_mm_and_ps(m, signs.f128));
}
