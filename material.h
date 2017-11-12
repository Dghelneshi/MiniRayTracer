#pragma once

#include "scene_object.h"
#include "pcg.h"
#include "math.h" // pow
#include "texture.h"
#include "onb.h"
#include "pdf.h"
#include "mrt_math.h"


struct scatter_record {
    ray specular_ray;
    Vec3 attenuation;
    bool is_specular;
};

class material {
public:
    virtual bool scatter(const ray& r_in, const hit_record& hrec, scatter_record *srec, pdf *pdf_storage) const {
        return false;
    }
    virtual float scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered) const {
        return 0;
    }
    virtual Vec3 sampleEmissive(const ray& r_in, const hit_record& rec) const {
        return Vec3(0.0f);
    }
    virtual ~material() = default;
};

////////////// LAMBERTIAN //////////////

class lambertian final : public material {
public:
    texture *albedo;

    lambertian(texture *albedo) : albedo(albedo) {};

    float scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered) const override {
        float cosine = dot(rec.n, scattered.dir);
        if (cosine < 0)
            return 0;
        else
            return cosine * (1.0f / M_PI_F);
    }

    bool scatter(const ray& r_in, const hit_record& hrec, scatter_record *srec, pdf *pdf_storage) const override {
        srec->is_specular = false;
        srec->attenuation = albedo->sample(hrec.u, hrec.v, hrec.p);
        new (pdf_storage) cosine_pdf(hrec.n);
        return true;
    }
};

////////////// ISOTROPIC //////////////

class isotropic final : public material {
public:
    texture *albedo;

    isotropic(texture *albedo) : albedo(albedo) {};
    
    float scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered) const override {
        return 1.0f / (2.0f * M_PI_F);
    }

    bool scatter(const ray& r_in, const hit_record& hrec, scatter_record *srec, pdf *pdf_storage) const override {
        srec->is_specular = false;
        srec->attenuation = albedo->sample(hrec.u, hrec.v, hrec.p);
        new (pdf_storage) isotropic_pdf(hrec.n);
        return true;
    }
};


//////////////// METAL ////////////////

class metal final : public material {
public:
    texture *albedo;
    float gloss;

    metal(texture* albedo, float gloss) : albedo(albedo) {
        this->gloss = std::min(gloss, 1.0f);
    }

    float scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered) const override {
        return 0;
    }
    bool scatter(const ray& r_in, const hit_record& hrec, scatter_record *srec, pdf *pdf_storage) const override {

        Vec3 reflected = reflect(r_in.dir, hrec.n);
        srec->specular_ray = ray(hrec.p, reflected + (1 - gloss) * random_in_sphere(), r_in.time); //TODO: fix direction, could be (0,0,0).
        srec->is_specular = true;
        srec->attenuation = albedo->sample(hrec.u, hrec.v, hrec.p);
        return true;
    }
};

////////////// DIELECTRIC //////////////

// cosine is cosI for ni < nt, otherwise cosT
// see https://graphics.stanford.edu/courses/cs148-10-summer/docs/2006--degreve--reflection_refraction.pdf
//
inline float fresnel_schlick(float cosine, float ref_index) {
    float r0 = (1 - ref_index) / (1 + ref_index); // r0 will be the same even if we swap ni and nt
    r0 = r0*r0;
    return r0 + (1 - r0) * pow((1 - cosine), 5);
}

class dielectric final : public material {
public:
    float ref_index;
    
    dielectric(float ref_index) : ref_index(ref_index) {}

    float scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered) const override {
        return 0;
    }
    bool scatter(const ray& r_in, const hit_record& hrec, scatter_record *srec, pdf *pdf_storage) const override {
       
        srec->attenuation = Vec3(1.0f);
        srec->is_specular = true;
        
        Vec3 facing_normal;
        float ni_over_nt;
        float cosI = -dot(r_in.dir, hrec.n);

        if (cosI < 0) {
            facing_normal = -hrec.n;
            ni_over_nt = ref_index;
        }
        else {
            facing_normal = hrec.n;
            ni_over_nt = 1.0f / ref_index;
        }

        Vec3 refracted;
        float reflect_prob = 1.0f;

        if (refract(r_in.dir, facing_normal, ni_over_nt, &refracted)) {

            float cosine_schlick;
            if (cosI < 0)
                cosine_schlick = MRT::sqrt(1.0f - ni_over_nt * ni_over_nt * (1.0f - cosI*cosI)); // cosine of refracted angle (cosT)
            else
                cosine_schlick = cosI;

            reflect_prob = fresnel_schlick(cosine_schlick, ref_index);
        }
        else {
            // always reflect if not refracted
            srec->specular_ray = ray(hrec.p, reflect(r_in.dir, hrec.n), r_in.time, r_in.isInside);
            return true;
        }
        
        if (randf() < reflect_prob) {
            srec->specular_ray = ray(hrec.p, reflect(r_in.dir, hrec.n), r_in.time, r_in.isInside);
        }
        else {
            int inside = r_in.isInside;
            if (cosI < 0) {
                inside--; // if we hit a backface we are now outside this volume

                // this can happen in very rare cases near intersections of objects or if we abuse zero-volume geometry like planes/rects
                if (inside < 0) inside = 0;
            }
            else {
                inside++; // if we hit a frontface we are now inside this volume
            }
            srec->specular_ray = ray(hrec.p, refracted, r_in.time, inside);
        }
        return true;
    }
};

//////////// DIFFUSE LIGHT ////////////

class diffuse_light final : public material {
public:
    texture *emissive;
    float scale;

    diffuse_light(texture *emissive, float scale = 1.0f) : emissive(emissive), scale(scale) {};

    float scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered) const override {
        return 0;
    }
    bool scatter(const ray& r_in, const hit_record& hrec, scatter_record *srec, pdf *pdf_storage) const override {
        return false;
    }
    Vec3 sampleEmissive(const ray& r_in, const hit_record& rec) const override {
        
        if (dot(rec.n, r_in.dir) < 0.0f)
            return scale * emissive->sample(rec.u, rec.v, rec.p);
        else
            return Vec3(0.0f);
    }
};

