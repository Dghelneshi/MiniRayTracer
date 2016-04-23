#pragma once

#include "scene_object.h"
#include "pcg.h"
#include "math.h" // pow
#include "texture.h"
#include "onb.h"
#include "pdf.h"

struct scatter_record {
    ray specular_ray;
    vec3 attenuation;
    pdf *pdf;
    bool is_specular;
};

class material {
public:
    virtual bool scatter(const ray& r_in, const hit_record& hrec, scatter_record *srec, pcg32_random_t* rng) const {
        return false;
    }
    virtual float scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered) const {
        return 0;
    }
    virtual vec3 sampleEmissive(const ray& r_in, const hit_record& rec) const {
        return vec3(0, 0, 0);
    }
    virtual ~material() {};
};

////////////// LAMBERTIAN //////////////

class lambertian : public material {
public:
    texture *albedo;

    lambertian(texture *albedo) : albedo(albedo) {};

    virtual float scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered) const override {
        float cosine = dot(rec.n, scattered.dir);
        if (cosine < 0)
            return 0;
        else
            return cosine / M_PI_F;
    }

    virtual bool scatter(const ray& r_in, const hit_record& hrec, scatter_record *srec, pcg32_random_t* rng) const override {
        srec->is_specular = false;
        srec->attenuation = albedo->sample(hrec.u, hrec.v, hrec.p);
        srec->pdf = new cosine_pdf(hrec.n); // FIXME
        return true;
    }
};

////////////// ISOTROPIC //////////////

class isotropic : public material {
public:
    texture *albedo;

    isotropic(texture *albedo) : albedo(albedo) {};
    
    virtual float scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered) const override {
        return 1 / (2 * M_PI_F);
    }

    virtual bool scatter(const ray& r_in, const hit_record& hrec, scatter_record *srec, pcg32_random_t* rng) const override {
        srec->is_specular = false;
        srec->attenuation = albedo->sample(hrec.u, hrec.v, hrec.p);
        srec->pdf = new isotropic_pdf(hrec.n); // FIXME
        return true;
    }
};


//////////////// METAL ////////////////

class metal : public material {
public:
    texture *albedo;
    float gloss;

    metal(texture* albedo, float gloss) : albedo(albedo) {
        this->gloss = min(gloss, 1);
    }

    virtual float scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered) const override {
        return 0;
    }
    virtual bool scatter(const ray& r_in, const hit_record& hrec, scatter_record *srec, pcg32_random_t* rng) const override {

        vec3 reflected = reflect(r_in.dir, hrec.n);
        srec->specular_ray = ray(hrec.p, reflected + (1 - gloss) * random_in_sphere(rng), r_in.time);
        srec->is_specular = true;
        srec->attenuation = albedo->sample(hrec.u, hrec.v, hrec.p);
        srec->pdf = nullptr;
        return true;
    }
};

////////////// DIELECTRIC //////////////

// cosine is cosI for ni < nt, otherwise cosT
// see https://graphics.stanford.edu/courses/cs148-10-summer/docs/2006--degreve--reflection_refraction.pdf
//
float fresnel_schlick(float cosine, float ref_index) {
    float r0 = (1 - ref_index) / (1 + ref_index); // r0 will be the same even if we swap ni and nt
    r0 = r0*r0;
    return r0 + (1 - r0) * pow((1 - cosine), 5);
}

class dielectric : public material {
public:
    float ref_index;
    
    dielectric(float ref_index) : ref_index(ref_index) {}

    virtual float scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered) const override {
        return 0;
    }
    virtual bool scatter(const ray& r_in, const hit_record& hrec, scatter_record *srec, pcg32_random_t* rng) const override {
       
        srec->attenuation = vec3(1.0f, 1.0f, 1.0f);
        srec->is_specular = true;
        
        vec3 facing_normal;
        float ni_over_nt;
        float cosI = -dot(r_in.dir, hrec.n);

        if (cosI < 0) { // we are inside the volume
            facing_normal = -hrec.n;
            ni_over_nt = ref_index;
        }
        else {
            facing_normal = hrec.n;
            ni_over_nt = 1.0f / ref_index;
        }

        vec3 refracted;
        float reflect_prob = 1.0f;

        if (refract(r_in.dir, facing_normal, ni_over_nt, &refracted)) {

            float cosine_schlick;
            if (cosI < 0)
                cosine_schlick = sqrt(1.0f - ni_over_nt * ni_over_nt * (1.0f - cosI*cosI)); // cosine of refracted angle (cosT)
            else
                cosine_schlick = cosI;

            reflect_prob = fresnel_schlick(cosine_schlick, ref_index);
        }
        else {
            // always reflect if not refracted
            srec->specular_ray = ray(hrec.p, reflect(r_in.dir, hrec.n), r_in.time);
            return true;
        }

        if (randf(rng) < reflect_prob) {
            srec->specular_ray = ray(hrec.p, reflect(r_in.dir, hrec.n), r_in.time);
        }
        else {
            srec->specular_ray = ray(hrec.p, refracted, r_in.time);
        }
        return true;
    }
};

//////////// DIFFUSE LIGHT ////////////

class diffuse_light : public material {
public:
    texture *emissive;
    float scale;

    diffuse_light(texture *emissive, float scale = 1.0f) : emissive(emissive), scale(scale) {};

    virtual float scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered) const override {
        return 0;
    }
    virtual bool scatter(const ray& r_in, const hit_record& hrec, scatter_record *srec, pcg32_random_t* rng) const override {
        return false;
    }
    virtual vec3 sampleEmissive(const ray& r_in, const hit_record& rec) const override {
        
        if (dot(rec.n, r_in.dir) < 0.0f)
            return scale * emissive->sample(rec.u, rec.v, rec.p);
        else
            return vec3(0, 0, 0);
    }
};

