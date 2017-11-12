#include "bench.h"
#include "vec3.h"
#include "pcg.h"

#define ASIZE (1024)

enum DP_Variants {
    DP_DPPS,
    DP_Simple,
    DP_Permute,
    DP_Shift,
    DP_Shuffle,
    DP_Movehl
};

template<DP_Variants I>
static void Vec3_Dot(benchmark::State& state) {
    Init_Thread_RNG(0x1234567890ABCDEF, 0xFEDCBA0987654321);
    Vec3 array[ASIZE];
    for (size_t i = 0; i < ASIZE; i++) {
        array[i] = Vec3(randf(), randf(), randf());
    }
    volatile float a;
    for (auto _ : state) {
        for (size_t i = 0; i < ASIZE; i += 2) {
            IACA_START;
            Vec3 &v1 = array[i + 0];
            Vec3 &v2 = array[i + 1];
            switch (I) {
                case DP_DPPS: {
                    a = _mm_cvtss_f32(_mm_dp_ps(v1.m, v2.m, 0x71));
                } break;

                case DP_Simple: {

                    a = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
                } break;

                case DP_Permute: {
                    __m128 xyz = _mm_mul_ps(v1.m, v2.m);
                    __m128 y = _mm_permute_ps(xyz, _MM_SHUFFLE(W, W, W, Y));
                    __m128 z = _mm_permute_ps(xyz, _MM_SHUFFLE(W, W, W, Z));
                    a = _mm_cvtss_f32(_mm_add_ss(_mm_add_ss(xyz, y), z));
                } break;

                case DP_Shift: {
                    m128 xyz = _mm_mul_ps(v1.m, v2.m);
                    m128 y = _mm_srli_si128(xyz, 4);
                    m128 z = _mm_srli_si128(xyz, 8);
                    a = _mm_cvtss_f32(_mm_add_ss(_mm_add_ss(xyz, y), z));
                } break;

                case DP_Shuffle: {
                    __m128 xyz = _mm_mul_ps(v1.m, v2.m);
                    __m128 y = _mm_shuffle_ps(xyz, xyz, _MM_SHUFFLE(W, W, W, Y));
                    __m128 z = _mm_shuffle_ps(xyz, xyz, _MM_SHUFFLE(W, W, W, Z));
                    a = _mm_cvtss_f32(_mm_add_ss(_mm_add_ss(xyz, y), z));
                } break;

                case DP_Movehl: {
                    m128 xyz = _mm_mul_ps(v1.m, v2.m);
                    __m128 y = _mm_permute_ps(xyz, _MM_SHUFFLE(W, W, W, Y));
                    __m128 z = _mm_movehl_ps(xyz, xyz);
                    a = _mm_cvtss_f32(_mm_add_ss(_mm_add_ss(xyz, y), z));
                } break;
            }
        }
        IACA_END;
    }
}

#if !defined(ENABLE_IACA) || defined(ENABLE_BENCH_VEC3)
BENCHMARK_MRT(Vec3_Dot, DP_DPPS);
BENCHMARK_MRT(Vec3_Dot, DP_Simple);
BENCHMARK_MRT(Vec3_Dot, DP_Permute);
BENCHMARK_MRT(Vec3_Dot, DP_Shift);
BENCHMARK_MRT(Vec3_Dot, DP_Shuffle);
BENCHMARK_MRT(Vec3_Dot, DP_Movehl);
#endif