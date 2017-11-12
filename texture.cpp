#include "vec3.h"
#include "math.h" // sin, floor, abs
#include "pcg.h"
#include "texture.h"


Vec3 checker_tex::sample(float u, float v, const Vec3& p) const {
#if 1
    float sines = sin(scale * p.x) * sin(scale * p.y) * sin(scale * p.z);
    if (sines < 0)
        return odd->sample(u, v, p);
    else
        return even->sample(u, v, p);
#else       
    float u_ = scale * u;
    float v_ = scale * v;
    int32 select = int32(floor(u_) + floor(v_));

    if (select & 1) {
        return odd->sample(u, v, p);
    }
    else
        return even->sample(u, v, p);
#endif
}


////////////////////////
//    PERLIN NOISE    //
////////////////////////

#if 0
static float trilerp(float c[2][2][2], float u, float v, float w) {
    Vec3 uvw(u, v, w);

    // manually unrolled loop with i,j,k from 0 to 1 each
    float acc = 0;
    Vec3 ijk1(0, 0, 0);
    Vec3 ijk2(0, 0, 1);
    Vec3 ijk3(0, 1, 0);
    Vec3 ijk4(0, 1, 1);
    for (int i = 0; i < 2; i++) {
        {
            Vec3 a = ijk1 * uvw + (Vec3(1) - ijk1) * (Vec3(1) - uvw);
            acc += a.x * a.y * a.z * c[i][0][0];
        }
        {
            Vec3 a = ijk2 * uvw + (Vec3(1) - ijk2) * (Vec3(1) - uvw);
            acc += a.x * a.y * a.z * c[i][0][1];
        }
        {
            Vec3 a = ijk3 * uvw + (Vec3(1) - ijk3) * (Vec3(1) - uvw);
            acc += a.x * a.y * a.z * c[i][1][0];
        }
        {
            Vec3 a = ijk4 * uvw + (Vec3(1) - ijk4) * (Vec3(1) - uvw);
            acc += a.x * a.y * a.z * c[i][1][1];
        }
        ijk1 += Vec3(1, 0, 0);
        ijk2 += Vec3(1, 0, 0);
        ijk3 += Vec3(1, 0, 0);
        ijk4 += Vec3(1, 0, 0);
    }
    return acc;
}
#endif

static float perlin_interp(Vec3 c[2][2][2], float u, float v, float w) {
    Vec3 uvw_init(u, v, w);
    Vec3 uvw = uvw_init * uvw_init * (Vec3(3) - 2 * uvw_init);

    // manually unrolled loop with i,j,k from 0 to 1 each
    float acc = 0;
    Vec3 ijk1(0, 0, 0);
    Vec3 ijk2(0, 0, 1);
    Vec3 ijk3(0, 1, 0);
    Vec3 ijk4(0, 1, 1);
    for (int i = 0; i < 2; i++) {
        {
            Vec3 weights = uvw_init - ijk1;
            Vec3 a = ijk1 * uvw + (Vec3(1) - ijk1) * (Vec3(1) - uvw);
            acc += a.x * a.y * a.z * dot(c[i][0][0], weights);
        }
        {
            Vec3 weights = uvw_init - ijk2;
            Vec3 a = ijk2 * uvw + (Vec3(1) - ijk2) * (Vec3(1) - uvw);
            acc += a.x * a.y * a.z * dot(c[i][0][1], weights);
        }
        {
            Vec3 weights = uvw_init - ijk3;
            Vec3 a = ijk3 * uvw + (Vec3(1) - ijk3) * (Vec3(1) - uvw);
            acc += a.x * a.y * a.z * dot(c[i][1][0], weights);
        }
        {
            Vec3 weights = uvw_init - ijk4;
            Vec3 a = ijk4 * uvw + (Vec3(1) - ijk4) * (Vec3(1) - uvw);
            acc += a.x * a.y * a.z * dot(c[i][1][1], weights);
        }
        ijk1 += Vec3(1, 0, 0);
        ijk2 += Vec3(1, 0, 0);
        ijk3 += Vec3(1, 0, 0);
        ijk4 += Vec3(1, 0, 0);
    }
    return acc;
}

#define PERLIN_COUNT (1 << 8)

static Vec3 ranvec[PERLIN_COUNT];
static int perm_x[PERLIN_COUNT];
static int perm_y[PERLIN_COUNT];
static int perm_z[PERLIN_COUNT];

float perlin_noise::noise(const Vec3& p) const {

    float u = p.x - floor(p.x);
    float v = p.y - floor(p.y);
    float w = p.z - floor(p.z);

    int i = (int) floor(p.x);
    int j = (int) floor(p.y);
    int k = (int) floor(p.z);

    Vec3 c[2][2][2];
#ifdef __clang__
    for (int di = 0; di < 2; di++) {
        for (int dj = 0; dj < 2; dj++) {
            for (int dk = 0; dk < 2; dk++) {
                c[di][dj][dk] = ranvec[perm_x[(i + di) & (PERLIN_COUNT - 1)] ^ perm_y[(j + dj) & (PERLIN_COUNT - 1)] ^ perm_z[(k + dk) & (PERLIN_COUNT - 1)]];
            }
        }
    }
#else
    int x0 = perm_x[(i + 0) & (PERLIN_COUNT - 1)];
    int y0 = perm_y[(j + 0) & (PERLIN_COUNT - 1)];
    int z0 = perm_z[(k + 0) & (PERLIN_COUNT - 1)];
    int x1 = perm_x[(i + 1) & (PERLIN_COUNT - 1)];
    int y1 = perm_y[(j + 1) & (PERLIN_COUNT - 1)];
    int z1 = perm_z[(k + 1) & (PERLIN_COUNT - 1)];

    c[0][0][0] = ranvec[x0 ^ y0 ^ z0];
    c[0][0][1] = ranvec[x0 ^ y0 ^ z1];
    c[0][1][0] = ranvec[x0 ^ y1 ^ z0];
    c[0][1][1] = ranvec[x0 ^ y1 ^ z1];

    c[1][0][0] = ranvec[x1 ^ y0 ^ z0];
    c[1][0][1] = ranvec[x1 ^ y0 ^ z1];
    c[1][1][0] = ranvec[x1 ^ y1 ^ z0];
    c[1][1][1] = ranvec[x1 ^ y1 ^ z1];
#endif

    return perlin_interp(c, u, v, w);
}

float perlin_noise::turbulence(const Vec3& p, int depth) const {
    float acc = 0;
    Vec3 p_copy = p;
    float weight = 1.0f;
    for (int i = 0; i < depth; i++) {
        acc += weight * noise(p_copy);
        weight *= 0.5f;
        p_copy *= 2;
    }
    return MRT::abs(acc);
}

static Vec3* perlin_generate() {
    for (int i = 0; i < PERLIN_COUNT; ++i) {
        ranvec[i] = random_in_sphere_g();
    }
    return ranvec;
}

static void permute(int *p, int n) {
    for (int i = n - 1; i > 0; i--) {
        int target = int(randf_g() * (i + 1));
        int tmp = p[i];
        p[i] = p[target];
        p[target] = tmp;
    }
}

static int* perlin_generate_perm(int select) {
    int* p = nullptr;
    switch (select) {
    case 0: p = perm_x; break;
    case 1: p = perm_y; break;
    case 2: p = perm_z; break;
    }

    for (int i = 0; i < PERLIN_COUNT; ++i) {
        p[i] = i;
    }
    permute(p, PERLIN_COUNT);

    return p;
}

// TODO does this work?
Vec3 *rv = perlin_generate();
int *px = perlin_generate_perm(0);
int *py = perlin_generate_perm(1);
int *pz = perlin_generate_perm(2);

/////////////////////////////////////////////////////////////////

Vec3 image_tex::sample(float u, float v, const Vec3& p) const {

    int32 i = int32((u) *  width);
    int32 j = int32((1 - v) * height);

    i = MRT::clamp(i, 0, width - 1);
    j = MRT::clamp(j, 0, height - 1);

    constexpr int bpp = 3;
    constexpr float f = (1.0f / 255.0f);

    size_t index = (i + width*j) * bpp;

    Vec3 result;
    result.r = data[index + 0];
    result.g = data[index + 1];
    result.b = data[index + 2];
    return result * f;
}