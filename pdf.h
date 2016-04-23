#pragma once

#include "vec3.h"
#include "onb.h"
#include "common.h"
#include "pcg.h"

class pdf {
public:
    virtual float value(const vec3& dir, float time) const = 0;
    virtual vec3 generate(float time, pcg32_random_t *rng) const = 0;

    virtual ~pdf() {};
};

class cosine_pdf : public pdf {
public:
    onb uvw;

    cosine_pdf(const vec3& n) : uvw(n) {}
    
    virtual float value(const vec3& dir, float time) const override {
        float cosine = dot(dir, uvw.w);
        if (cosine > 0)
            return cosine / M_PI_F;
        else
            return 0;
    }
    virtual vec3 generate(float time, pcg32_random_t *rng) const override {
        return uvw * random_cosine_direction(rng);
    }
};

class isotropic_pdf : public pdf {
public:

    isotropic_pdf(const vec3& n) {}

    virtual float value(const vec3& dir, float time) const override {
        return 1 / (2 * M_PI_F);
    }
    virtual vec3 generate(float time, pcg32_random_t *rng) const override {
        return random_in_sphere(rng);
    }
};

class object_pdf : public pdf {
public:
    vec3 origin;
    scene_object *obj;

    object_pdf(const vec3& origin, scene_object *obj) : origin(origin), obj(obj) {}

    virtual float value(const vec3& dir, float time) const override {
        return obj->pdf_value(origin, dir, time);
    }
    virtual vec3 generate(float time, pcg32_random_t *rng) const override {
        return obj->pdf_generate(origin, time, rng);
    }
};

class mix_pdf : public pdf {
public:
    pdf *p0;
    pdf *p1;

    mix_pdf(pdf *p0, pdf *p1) : p0(p0), p1(p1) {}

    virtual float value(const vec3& dir, float time) const override {
        return 0.5f * (p0->value(dir, time) + p1->value(dir, time));
    }
    virtual vec3 generate(float time, pcg32_random_t *rng) const override {
        if (randf(rng) < 0.5f)
            return p0->generate(time, rng);
        else
            return p1->generate(time, rng);
    }
};