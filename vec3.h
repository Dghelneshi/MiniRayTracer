#pragma once
#include <math.h>

class vec3 {
public:
    union {
        float e[3];
        struct {
            float x;
            float y;
            float z;
        };
        struct {
            float r;
            float g;
            float b;
        };
    };

    vec3() = default;
    vec3(float x, float y, float z) { 
        e[0] = x;
        e[1] = y;
        e[2] = z; 
    }

    inline vec3 operator-() const { 
        return vec3(-x, -y, -z);
    }

    inline float operator[](int i) const {
        return e[i];
    }

    inline float& operator[](int i) {
        return e[i];
    }

    inline float length() const;
    inline float sdot() const;
    inline vec3 normalize();
    inline vec3 gamma_correct();
    inline vec3 inv_gamma_correct();
    inline vec3 reflect(const vec3& n);

};

inline vec3 operator+(const vec3& v1, const vec3& v2) {
    return vec3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}
inline vec3 operator-(const vec3& v1, const vec3& v2) {
    return vec3(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

inline vec3 operator*(const vec3& v, const float f) {
    return vec3(v.x * f, v.y * f, v.z * f);
}
inline vec3 operator*(const float f, const vec3& v) {
    return vec3(v.x * f, v.y * f, v.z * f);
}
inline vec3 operator/(const vec3& v, const float f) {
    return vec3(v.x / f, v.y / f, v.z / f);
}

inline vec3& operator+=(vec3 &v1, const vec3& v2) {
    v1 = v1 + v2;
    return v1;
}
inline vec3& operator-=(vec3 &v1, const vec3& v2) {
    v1 = v1 - v2;
    return v1;
}

inline vec3& operator*=(vec3 &v1, const float f) { 
    v1 = v1 * f;
    return v1;
}
inline vec3& operator/=(vec3 &v1, const float f) {
    v1 = v1 / f;
    return v1;
}

// element wise mul and div, mostly for colors

inline vec3 operator*(const vec3& v1, const vec3& v2) {
    return vec3(v1.r * v2.r, v1.g * v2.g, v1.b * v2.b);
}
inline vec3 operator/(const vec3& v1, const vec3& v2) {
    return vec3(v1.r / v2.r, v1.g / v2.g, v1.b / v2.b);
}
inline vec3& operator*=(vec3 &v1, const vec3& v2) {
    v1 = v1 * v2;
    return v1;
}
inline vec3& operator/=(vec3 &v1, const vec3& v2) {
    v1 = v1 / v2;
    return v1;
}

////

inline float dot(const vec3& v1, const vec3& v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

inline vec3 cross(const vec3& v1, const vec3& v2) {
    return vec3(v1.y * v2.z - v1.z * v2.y,
                v1.z * v2.x - v1.x * v2.z,
                v1.x * v2.y - v1.y * v2.x);
}

inline float sdot(const vec3& v) {
    return dot(v, v);
}

inline float vec3::sdot() const {
    return dot(*this, *this);
}

inline float vec3::length() const {
     return sqrt(sdot());
}

inline vec3 unit_vector(const vec3& v) {
    return v / v.length();
}

inline vec3 vec3::normalize() {
     *this /= this->length();
     return *this;
}

#define MRT_GAMMA 2.2f

inline vec3 vec3::gamma_correct() {
    //r = sqrt(r);
    //g = sqrt(g);
    //b = sqrt(b);
    r = pow(r, 1 / MRT_GAMMA);
    g = pow(g, 1 / MRT_GAMMA);
    b = pow(b, 1 / MRT_GAMMA);
    return *this;
}

inline vec3 gamma_correct(const vec3& c) {
    return vec3(pow(c.r, 1 / MRT_GAMMA), pow(c.g, 1 / MRT_GAMMA), pow(c.b, 1 / MRT_GAMMA));
}

inline vec3 vec3::inv_gamma_correct() {
    //r *= r;
    //g *= g;
    //b *= b;
    r = pow(r, MRT_GAMMA);
    g = pow(g, MRT_GAMMA);
    b = pow(b, MRT_GAMMA);
    return *this;
}

inline vec3 inv_gamma_correct(const vec3& c) {
    //return vec3(c.r * c.r, c.g * c.g, c.b * c.b);
    return vec3(pow(c.r, MRT_GAMMA), pow(c.g, MRT_GAMMA), pow(c.b, MRT_GAMMA));
}

inline vec3 vmin(const vec3& a, const vec3& b) {
    float xmin = min(a.x, b.x);
    float ymin = min(a.y, b.y);
    float zmin = min(a.z, b.z);
    return vec3(xmin, ymin, zmin);
}

inline vec3 vmin(const vec3& a, const vec3& b, const vec3& c) {
    return vmin(vmin(a, b), c);
}

inline vec3 vmax(const vec3& a, const vec3& b) {
    float xmax = max(a.x, b.x);
    float ymax = max(a.y, b.y);
    float zmax = max(a.z, b.z);
    return vec3(xmax, ymax, zmax);
}

inline vec3 vmax(const vec3& a, const vec3& b, const vec3& c) {
    return vmax(vmax(a, b), c);
}


inline vec3 vec3::reflect(const vec3& n) {
    *this = *this - (2 * dot(*this, n) * n);
    return *this;
}

inline vec3 reflect(const vec3& v, const vec3& n) {
    return v - (2 * dot(v, n) * n);
}

// refracted vector not normalized!
bool refract(const vec3& v, const vec3& n, float ni_over_nt, vec3* refracted) {
    float cosI = -dot(v, n);
    float sinT2 = ni_over_nt * ni_over_nt * (1.0f - cosI*cosI);
    
    if (sinT2 <= 1.0f) {
        float cosT = sqrt(1.0f - sinT2);
        *refracted = ni_over_nt * v + (ni_over_nt * cosI - cosT) * n;
        return true;
    }
    else { // total inner reflection
        return false;
    }
}