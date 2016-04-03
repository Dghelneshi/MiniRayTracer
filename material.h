#pragma once

#include "scene_object.h"
#include "pcg.h"
#include "math.h" // pow
#include "texture.h"

class material {
public:
    virtual bool scatter(const ray& r_in, const hit_record& rec, vec3 *attenuation, pcg32_random_t* rng, ray *scattered) const = 0;
    virtual ~material() {};
};

////////////////////////////////////

class lambertian : public material {
public:
    texture *albedo;

    lambertian(texture *albedo) : albedo(albedo) {};

    virtual bool scatter(const ray& r_in, const hit_record& rec, vec3 *attenuation, pcg32_random_t* rng, ray *scattered) const {
        vec3 target = rec.p + rec.n + random_in_sphere(rng);
        *scattered = ray(rec.p, target - rec.p);
        *attenuation = albedo->sample(0, 0, rec.p);
        return true;
    }
};

////////////////////////////////////

class metal : public material {
public:
    vec3 albedo;
    float gloss;

    metal(const vec3& albedo, float gloss) : albedo(albedo) {
        this->gloss = min(gloss, 1);
    }

    virtual bool scatter(const ray& r_in, const hit_record& rec, vec3 *attenuation, pcg32_random_t* rng, ray *scattered) const {
        vec3 reflected = reflect(r_in.dir, rec.n);
        *scattered = ray(rec.p, reflected + (1-gloss) * random_in_sphere(rng));
        *attenuation = albedo;
        return (dot(scattered->dir, rec.n) > 0);
    }
};

////////////////////////////////////

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

    virtual bool scatter(const ray& r_in, const hit_record& rec, vec3 *attenuation, pcg32_random_t* rng, ray *scattered) const {
        
        *attenuation = vec3(1.0f, 1.0f, 1.0f);
        
        vec3 facing_normal;
        float ni_over_nt;
        float cosI = -dot(r_in.dir, rec.n);

        if (cosI < 0) { // we are inside the volume
            facing_normal = -rec.n;
            ni_over_nt = ref_index;
        }
        else {
            facing_normal = rec.n;
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
            *scattered = ray(rec.p, reflect(r_in.dir, rec.n));
            return true;
        }

        if (randf(rng) < reflect_prob) {
            *scattered = ray(rec.p, reflect(r_in.dir, rec.n));
        }
        else {
            *scattered = ray(rec.p, refracted);
        }
        return true;
    }
};