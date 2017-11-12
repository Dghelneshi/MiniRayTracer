#include "bench.h"
#include "mat4.h"
#include "pcg.h"

#define ASIZE (1024)

inline static Vec4 rv() {
    return Vec4(randf(), randf(), randf(), randf());
}

enum RowVec4Mul_Variants {
    RM_DPPS,
    RM_Unpack,
    RM_Permute
};

template<RowVec4Mul_Variants I>
static void Mat4_RowVec4Mul(benchmark::State& state) {
    Init_Thread_RNG(0x1234567890ABCDEF, 0xFEDCBA0987654321);
    Vec4 va[ASIZE];
    Mat4 ma[ASIZE];
    for (size_t i = 0; i < ASIZE; i++) {
        va[i] = rv();
        ma[i] = Mat4(rv(),rv(),rv(),rv());
    }
    volatile float a;
    for (auto _ : state) {
        for (size_t i = 0; i < ASIZE; i++) {
            IACA_START;
            Mat4 &m = ma[i];
            Vec4 &v = va[i];
            switch (I) {

            case RM_DPPS: {
                __m128 x = _mm_dp_ps(v.m, m.c0.m, 0xF1);
                __m128 y = _mm_dp_ps(v.m, m.c1.m, 0xF2);
                __m128 z = _mm_dp_ps(v.m, m.c2.m, 0xF4);
                __m128 w = _mm_dp_ps(v.m, m.c3.m, 0xF8);
                v = _mm_blend_ps(_mm_blend_ps(x, y, 0x2), _mm_blend_ps(z, w, 0x8), 0xC);
            } break;

            case RM_Unpack: {
                __m128 m0 = _mm_mul_ps(v.m, m[0].m);
                __m128 m1 = _mm_mul_ps(v.m, m[1].m);
                __m128 m2 = _mm_mul_ps(v.m, m[2].m);
                __m128 m3 = _mm_mul_ps(v.m, m[3].m);

                __m128 xxyy = _mm_unpacklo_ps(m0, m1); // m0x m1x m0y m1y
                __m128 zzww = _mm_unpackhi_ps(m0, m1); // m0z m1z m0w m1w
                __m128 a01 = _mm_add_ps(xxyy, zzww);  //  x + z | y + w

                xxyy = _mm_unpacklo_ps(m2, m3); // m2x m3x m2y m3y
                zzww = _mm_unpackhi_ps(m2, m3); // m2z m3z m2w m3w
                __m128 a23 = _mm_add_ps(xxyy, zzww);  //  x + z | y + w

                __m128 axz = _mm_movelh_ps(a01, a23);  // m01 x+z | m23 x+z
                __m128 ayw = _mm_movehl_ps(a23, a01);  // m01 y+w | m23 y+w
                __m128 res = _mm_add_ps(axz, ayw);     // x+y+z+w | x+y+z+w

                v = res;
            } break;

            case RM_Permute: {
                __m128 xyzw = _mm_mul_ps(v.m, m.c0.m);
                __m128 y = _mm_permute_ps(xyzw, _MM_SHUFFLE(W, W, W, Y));
                __m128 z = _mm_permute_ps(xyzw, _MM_SHUFFLE(W, W, W, Z));
                __m128 w = _mm_permute_ps(xyzw, _MM_SHUFFLE(W, W, W, W));
                __m128 rx = _mm_add_ss(_mm_add_ss(xyzw, y), _mm_add_ss(z, w));

                xyzw = _mm_mul_ps(v.m, m.c1.m);
                y = _mm_permute_ps(xyzw, _MM_SHUFFLE(W, W, W, Y));
                z = _mm_permute_ps(xyzw, _MM_SHUFFLE(W, W, W, Z));
                w = _mm_permute_ps(xyzw, _MM_SHUFFLE(W, W, W, W));
                __m128 ry = _mm_add_ss(_mm_add_ss(xyzw, y), _mm_add_ss(z, w));

                xyzw = _mm_mul_ps(v.m, m.c2.m);
                y = _mm_permute_ps(xyzw, _MM_SHUFFLE(W, W, W, Y));
                z = _mm_permute_ps(xyzw, _MM_SHUFFLE(W, W, W, Z));
                w = _mm_permute_ps(xyzw, _MM_SHUFFLE(W, W, W, W));
                __m128 rz = _mm_add_ss(_mm_add_ss(xyzw, y), _mm_add_ss(z, w));

                xyzw = _mm_mul_ps(v.m, m.c3.m);
                y = _mm_permute_ps(xyzw, _MM_SHUFFLE(W, W, W, Y));
                z = _mm_permute_ps(xyzw, _MM_SHUFFLE(W, W, W, Z));
                w = _mm_permute_ps(xyzw, _MM_SHUFFLE(W, W, W, W));
                __m128 rw = _mm_add_ss(_mm_add_ss(xyzw, y), _mm_add_ss(z, w));

                rz = _mm_insert_ps(rz, rw, 0x1C);                 // z w 0 0
                rz = _mm_permute_ps(rz, _MM_SHUFFLE(Y, X, W, W)); // 0 0 z w
                rx = _mm_insert_ps(rx, ry, 0x1C);                 // x y 0 0

                v = _mm_or_ps(rx, rz);
            } break;
            }
        }
        IACA_END;
    }
    for (size_t i = 0; i < ASIZE; i++) {
        a = sdot(va[i]);
    }
}


enum Transpose_Variants {
    T_Unpack,
    T_Shuf
};

template<Transpose_Variants I>
static void Mat4_Transpose(benchmark::State& state) {
    Init_Thread_RNG(0x1234567890ABCDEF, 0xFEDCBA0987654321);
    Mat4 ma[ASIZE];
    for (size_t i = 0; i < ASIZE; i++) {
        ma[i] = Mat4(rv(), rv(), rv(), rv());
    }
    volatile float a;
    __m128 tmp3, tmp2, tmp1, tmp0;
    for (auto _ : state) {
        for (size_t i = 0; i < ASIZE; i++) {
            IACA_START;
            Mat4 &m = ma[i];
            switch (I) {
            case T_Unpack: {
                tmp0 = _mm_unpacklo_ps(m.c0.m, m.c1.m); // c0x c1x c0y c1y
                tmp2 = _mm_unpacklo_ps(m.c2.m, m.c3.m); // c2x c3x c2y c3y
                tmp1 = _mm_unpackhi_ps(m.c0.m, m.c1.m); // c0z c1z c0w c1w
                tmp3 = _mm_unpackhi_ps(m.c2.m, m.c3.m); // c2z c3z c2w c3w
                m.c0 = _mm_movelh_ps(tmp0, tmp2);     // c0x c1x c2x c3x
                m.c1 = _mm_movehl_ps(tmp2, tmp0);     // c0y c1y c2y c3y
                m.c2 = _mm_movelh_ps(tmp1, tmp3);     // c0z c1z c2z c3z
                m.c3 = _mm_movehl_ps(tmp3, tmp1);     // c0w c1w c2w c3w
            } break;

            case T_Shuf: {
                tmp0 = _mm_shuffle_ps(m.c0.m, m.c1.m, 0x44);
                tmp2 = _mm_shuffle_ps(m.c0.m, m.c1.m, 0xEE);
                tmp1 = _mm_shuffle_ps(m.c2.m, m.c3.m, 0x44);
                tmp3 = _mm_shuffle_ps(m.c2.m, m.c3.m, 0xEE);

                m.c0 = _mm_shuffle_ps(tmp0, tmp1, 0x88);
                m.c1 = _mm_shuffle_ps(tmp0, tmp1, 0xDD);
                m.c2 = _mm_shuffle_ps(tmp2, tmp3, 0x88);
                m.c3 = _mm_shuffle_ps(tmp2, tmp3, 0xDD);
            } break;
            }
            ma[i] = m;
        }
        IACA_END;
    }
    for (size_t i = 0; i < ASIZE; i++) {
        a = sdot(ma[i][0]);
    }
}


#if !defined(ENABLE_IACA) || defined(ENABLE_BENCH_MAT4)
BENCHMARK_MRT(Mat4_RowVec4Mul, RM_DPPS);
BENCHMARK_MRT(Mat4_RowVec4Mul, RM_Unpack);
BENCHMARK_MRT(Mat4_RowVec4Mul, RM_Permute);
BENCHMARK_MRT(Mat4_Transpose, T_Unpack);
BENCHMARK_MRT(Mat4_Transpose, T_Shuf);
#endif