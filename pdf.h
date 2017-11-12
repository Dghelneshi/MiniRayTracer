#pragma once

#include "vec3.h"
#include "onb.h"
#include "common.h"
#include "pcg.h"
#include <type_traits>


class pdf {
public:
    virtual float value(const Vec3& dir, float time) const = 0;
    virtual Vec3 generate(float time) const = 0;

    virtual ~pdf() = default;
};

class cosine_pdf final : public pdf {
public:
    onb uvw;

    cosine_pdf(const Vec3& n) : uvw(n) {}

    float value(const Vec3& dir, float time) const override {
        float cosine = dot(dir, uvw.w);
        if (cosine > 0)
            return cosine / M_PI_F;
        else
            return 0;
    }
    Vec3 generate(float time) const override {
        return uvw * random_cosine_direction();
    }
};

class isotropic_pdf final : public pdf {
public:

    isotropic_pdf(const Vec3& n) {}

    float value(const Vec3& dir, float time) const override {
        return 1 / (2 * M_PI_F);
    }
    Vec3 generate(float time) const override {
        return random_in_sphere();
    }
};

class object_pdf final : public pdf {
public:
    Vec3 origin;
    scene_object *obj;

    object_pdf(const Vec3& origin, scene_object *obj) : origin(origin), obj(obj) {}

    float value(const Vec3& dir, float time) const override {
        return obj->pdf_value(origin, dir, time);
    }
    Vec3 generate(float time) const override {
        return obj->pdf_generate(origin, time);
    }
};

class mix_pdf final : public pdf {
public:
    pdf *p0;
    pdf *p1;

    mix_pdf(pdf *p0, pdf *p1) : p0(p0), p1(p1) {}

    float value(const Vec3& dir, float time) const override {
        return 0.5f * (p0->value(dir, time) + p1->value(dir, time));
    }
    Vec3 generate(float time) const override {
        if (randf() < 0.5f)
            return p0->generate(time);
        else
            return p1->generate(time);
    }
};

struct pdf_space {
    // NOTE: std::aligned_union/aligned_storage is not guaranteed to support alignment beyond 8 because C++ people are insane.
    //       We circumvent the problem by using the precalculated alignment value instead of the end result alignment from the aligned_storage implementation.
    using any_pdf = std::aligned_union<1, cosine_pdf, isotropic_pdf, object_pdf, mix_pdf>;
    alignas(any_pdf::alignment_value) any_pdf::type p;
};