#pragma once

#include "vec3.h"
#include "math.h" // sin, floor, abs

class texture {
public:
    virtual vec3 sample(float u, float v, const vec3& p) const = 0;
    virtual ~texture() {}
};

class color_tex : public texture {
public:
    vec3 color;

    color_tex() {}
    color_tex(vec3 c) : color(c) {}

    virtual vec3 sample(float u, float v, const vec3& p) const override {
        return color;
    }
};


class checker_tex : public texture {
public:
    texture *even;
    texture *odd;
    float scale;

    checker_tex(texture *t0, texture *t1, float scale) : even(t0), odd(t1), scale(scale) {}

    virtual vec3 sample(float u, float v, const vec3& p) const override {
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

};


////////////////////////
//    PERLIN NOISE    //
////////////////////////

inline float trilerp(float c[2][2][2], float u, float v, float w) {
    float acc = 0;
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            for (int k = 0; k < 2; k++)
            {
                acc += ((i * u + (1 - i)*(1 - u)) *
                        (j * v + (1 - j)*(1 - v)) *
                        (k * w + (1 - k)*(1 - w)) * c[i][j][k]);
            }
        }
    }
    return acc;
}


inline float perlin_interp(vec3 c[2][2][2], float u, float v, float w) {
    
    vec3 uvw(u, v, w);
    
    u = u*u*(3 - 2 * u);
    v = v*v*(3 - 2 * v);
    w = w*w*(3 - 2 * w);

    float acc = 0;
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            for (int k = 0; k < 2; k++)
            {
                vec3 weights = uvw - vec3(float(i), float(j), float(k));
                acc += ((i * u + (1 - i)*(1 - u)) *
                        (j * v + (1 - j)*(1 - v)) *
                        (k * w + (1 - k)*(1 - w)) * dot(c[i][j][k], weights));
            }
        }
    }
    return acc;
}


class perlin_noise {
public:
    static vec3 *ranvec;
    static int *perm_x;
    static int *perm_y;
    static int *perm_z;

    float noise(const vec3& p) const {
        float u = p.x - floor(p.x);
        float v = p.y - floor(p.y);
        float w = p.z - floor(p.z);

        int i = (int) floor(p.x);
        int j = (int) floor(p.y);
        int k = (int) floor(p.z);
        
        vec3 c[2][2][2];
        for (int di = 0; di < 2; di++) {
            for (int dj = 0; dj < 2; dj++) {
                for (int dk = 0; dk < 2; dk++) {
                    c[di][dj][dk] = ranvec[perm_x[(i + di) & 255] ^ perm_y[(j + dj) & 255] ^ perm_z[(k + dk) & 255]];
                }
            }
        }

        return perlin_interp(c, u, v, w);
    }

    float turbulence(const vec3& p, int depth = 7) const {
        float acc = 0;
        vec3 p_copy = p;
        float weight = 1.0f;
        for (int i = 0; i < depth; i++) {
            acc += weight * noise(p_copy);
            weight *= 0.5f;
            p_copy *= 2;
        }
        return std::abs(acc);
    }
};

static vec3 *perlin_generate() {
    vec3 *p = new vec3[256];
    
    for (int i = 0; i < 256; ++i) {
        p[i] = random_in_sphere();
    }
    return p;
}

static void permute(int *p, int n) {
    for (int i = n - 1; i > 0; i--) {
        int target = int(randf() * (i + 1));
        int tmp = p[i];
        p[i] = p[target];
        p[target] = tmp;
    }
}

static int* perlin_generate_perm() {
    int *p = new int[256];
    for (int i = 0; i < 256; ++i) {
        p[i] = i;
    }
    permute(p, 256);
    return p;
}

vec3 *perlin_noise::ranvec = perlin_generate();
int *perlin_noise::perm_x = perlin_generate_perm();
int *perlin_noise::perm_y = perlin_generate_perm();
int *perlin_noise::perm_z = perlin_generate_perm();


class perlin_tex : public texture {
public:
    perlin_noise noise;
    float scale;

    perlin_tex() : scale(1) {}
    perlin_tex(float scale) : scale(scale) {}

    virtual vec3 sample(float u, float v, const vec3& p) const override {
        //return vec3(1, 1, 1) * 0.5f * (1 + noise.noise(p*scale));
        return vec3(1, 1, 1) * noise.turbulence(p * scale);
        //return vec3(1, 1, 1) * 0.5f * (1 + noise.turbulence(p * scale));
        //return vec3(1, 1, 1)*0.5f*(1 + sin(scale * (p.length() + 10 * noise.turbulence(p))));
        //return vec3(1, 1, 1)*0.5f*(1 + sin(scale * p.z + 10 * noise.turbulence(p)));
    }
};


/////////////////////////////////////////////////////////////////

class image_tex : public texture {
public:
    uint8 *data;
    int width, height;

    image_tex(uint8 *pixels, int32 width, int32 height) : data(pixels), width(width), height(height) {}
    
    virtual vec3 sample(float u, float v, const vec3& p) const override;
};

vec3 image_tex::sample(float u, float v, const vec3& p) const {
    
    const int bpp = 3;

    int32 i = int32((    u) *  width);
    int32 j = int32((1 - v) * height);

    if (i < 0) i = 0;
    if (j < 0) j = 0;
    if (i > width  - 1) i = width  - 1;
    if (j > height - 1) j = height - 1;

    float r = data[(i + width*j)*bpp + 0] / 255.0f;
    float g = data[(i + width*j)*bpp + 1] / 255.0f;
    float b = data[(i + width*j)*bpp + 2] / 255.0f;
    return vec3(r, g, b);
}