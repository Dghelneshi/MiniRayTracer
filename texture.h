#pragma once

#include "vec3.h"

class texture {
public:
    virtual Vec3 sample(float u, float v, const Vec3& p) const = 0;
    virtual ~texture() {}
};

class color_tex final : public texture {
public:
    Vec3 color;

    color_tex() {}
    color_tex(const Vec3& c) : color(c) {}

    Vec3 sample(float u, float v, const Vec3& p) const override {
        return color;
    }
};


class checker_tex final : public texture {
public:
    texture *even;
    texture *odd;
    float scale;

    checker_tex(texture *t0, texture *t1, float scale) : even(t0), odd(t1), scale(scale) {}

    Vec3 sample(float u, float v, const Vec3& p) const override;

};


////////////////////////
//    PERLIN NOISE    //
////////////////////////

class perlin_noise {
public:
    float noise(const Vec3& p) const;
    float turbulence(const Vec3& p, int depth = 7) const;
};


class perlin_tex final : public texture {
public:
    perlin_noise noise;
    float scale;

    perlin_tex() : scale(1) {}
    perlin_tex(float scale) : scale(scale) {}

    Vec3 sample(float u, float v, const Vec3& p) const override {
        //return Vec3(1, 1, 1) * 0.5f * (1 + noise.noise(p*scale));
        return Vec3(1, 1, 1) * noise.turbulence(p * scale);
        //return Vec3(1, 1, 1) * 0.5f * (1 + noise.turbulence(p * scale));
        //return Vec3(1, 1, 1)*0.5f*(1 + sin(scale * (p.length() + 10 * noise.turbulence(p))));
        //return Vec3(1, 1, 1)*0.5f*(1 + sin(scale * p.z + 10 * noise.turbulence(p)));
    }
};


/////////////////////////////////////////////////////////////////

class image_tex final : public texture {
public:
    uint8 *data;
    int32 width, height;

    image_tex(uint8 *pixels, int32 width, int32 height) : data(pixels), width(width), height(height) {}
    
    Vec3 sample(float u, float v, const Vec3& p) const override;
};