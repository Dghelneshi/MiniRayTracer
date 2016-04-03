
#include "vec3.h"
#include "math.h" // sin, floor, abs

class texture {
public:
    virtual vec3 sample(float u, float v, const vec3& p) const = 0;
};

class uni_color_tex : public texture {
public:
    vec3 color;

    uni_color_tex() {}
    uni_color_tex(vec3 c) : color(c) {}

    virtual vec3 sample(float u, float v, const vec3& p) const {
        return color;
    }
};


class checker_tex : public texture {
public:
    texture *even;
    texture *odd;

    checker_tex() {}
    checker_tex(texture *t0, texture *t1) : even(t0), odd(t1) {}

    virtual vec3 sample(float u, float v, const vec3& p) const {
        float sines = sin(10 * p.x) * sin(10 * p.y) * sin(10 * p.z);
        if (sines < 0)
            return odd->sample(u, v, p);
        else
            return even->sample(u, v, p);
    }

};

//////////////////////////////////////////////////////////////

float trilerp(float c[2][2][2], float u, float v, float w) {
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


float perlin_interp(vec3 c[2][2][2], float u, float v, float w) {
    
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
                vec3 weights = uvw - vec3(i, j, k);
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

    virtual vec3 sample(float u, float v, const vec3& p) const {
        //return vec3(1, 1, 1) * 0.5f * (1 + noise.noise(p*scale)));
        //return vec3(1, 1, 1) * noise.turbulence(p * scale);
        //return vec3(1, 1, 1) * 0.5f * (1 + noise.turbulence(p * scale));
        return vec3(1, 1, 1)*0.5f*(1 + sin(scale * p.length() + 10 * noise.turbulence(p)));
    }
};