#pragma once

#include "ray.h"
#include "aabb.h"
#include <stdlib.h> // qsort
#include <math.h> // INFINITY


class material;

struct hit_record {
    float t;
    vec3 p;
    vec3 n;
    float u;
    float v;
    material *mat_ptr;
};


class scene_object {
public:
    virtual bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const = 0;
    virtual bool bounding_box(aabb* box, float time0, float time1) const = 0;
    virtual float pdf_value(const vec3& origin, const vec3& dir, float time) const {
        return 0;
    }
    virtual vec3 pdf_generate(const vec3& origin, float time, pcg32_random_t *rng) const {
        return vec3(1, 0, 0);
    }

    virtual ~scene_object() {}
};


//////////////////////////////////////////////////////////////////////////////////////

// TODO: test invalid bbox code (for objects like planes)

class object_list : public scene_object {
public:
    scene_object **list;
    size_t count;
    aabb box;
    bool hasBox;

    object_list(scene_object* l[], size_t n, float time0, float time1);

    virtual bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const override;

    virtual bool bounding_box(aabb* b, float time0, float time1) const override {

        if (!hasBox) {
            return false;
        }
        else {
            *b = box;
            return true;
        }
    }

    virtual float pdf_value(const vec3& origin, const vec3& dir, float time) const override {
        float sum = 0;
        for (size_t i = 0; i < count; i++) {
            sum += list[i]->pdf_value(origin, dir, time);
        }
        return sum / count;
    }
    virtual vec3 pdf_generate(const vec3& origin, float time, pcg32_random_t *rng) const override {
        int i = int(randf(rng) * count);
        return list[i]->pdf_generate(origin, time, rng);
    }
};

bool object_list::hit(const ray& r, float tmin, float tmax, hit_record *rec) const {
    
    if (!hasBox || box.hit(r, tmin, tmax)) {
        hit_record cur_rec;
        bool hit = false;
        float closest = tmax;

        for (size_t i = 0; i < count; i++) {

            if (list[i]->hit(r, tmin, closest, &cur_rec)) {
                hit = true;
                closest = cur_rec.t;
                *rec = cur_rec;
            }
        }

        return hit;
    }
    else {
        return false;
    }

}

object_list::object_list(scene_object* l[], size_t n, float time0, float time1) {

    list = l;
    count = n;

    vec3 minbb(FLT_MAX, FLT_MAX, FLT_MAX);
    vec3 maxbb(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (size_t i = 0; i < count; i++)
    {
        aabb curbox;
        bool cur_hasBox = list[i]->bounding_box(&curbox, time0, time1);

        if (cur_hasBox) {
            for (int axis = 0; axis < 3; axis++)
            {
                minbb[axis] = min(minbb[axis], curbox._min[axis]);
                maxbb[axis] = max(maxbb[axis], curbox._max[axis]);
            }
        }
        else {
            hasBox = false;
            return;
        }
    }

    box = aabb(minbb, maxbb);
    hasBox = true;
}


///////////////////////////////////
//   BOUNDING VOLUME HIERARCHY   //
///////////////////////////////////

class bvh_node : public scene_object {
public:
    scene_object *left;
    scene_object *right;
    aabb box;
    
    bvh_node(scene_object* list[], int n, float time0, float time1);

    virtual bool bounding_box(aabb* b, float time0, float time1) const override {
        *b = box;
        return true;
    }
    virtual bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const override;
};

bool bvh_node::hit(const ray& r, float tmin, float tmax, hit_record *rec) const {

    if (box.hit(r, tmin, tmax)) {
        hit_record left_rec, right_rec;
        bool hit_left = left->hit(r, tmin, tmax, &left_rec);
        bool hit_right = right->hit(r, tmin, tmax, &right_rec);

        if (!(hit_left || hit_right)) {
            return false;
        }

        if (hit_left && hit_right) {
            if (left_rec.t < right_rec.t) {
                *rec = left_rec;
            }
            else {
                *rec = right_rec;
            }
        }
        else if (hit_left) {
            *rec = left_rec;
        }
        else if (hit_right) {
            *rec = right_rec;
        }

        return true;
    }
    else {
        return false;
    }
}


// couldn't figure out if there's a way to do this with macros/templates/something else instead of copy/paste
int box_compare_x(const void *a, const void *b) {
    
    aabb box_left, box_right;
    scene_object *a_obj = *(scene_object**) a;
    scene_object *b_obj = *(scene_object**) b;

    if (!(a_obj->bounding_box(&box_left, 0, 0) && b_obj->bounding_box(&box_right, 0, 0))) {
        OutputDebugStringA("no bounding box in bvh_node constructor\n");
        DebugBreak();
    }

    if (box_left._min.x - box_right._min.x == 0.0f) { // 0 / equal case must exist, can otherwise cause infinite loop on msvc!
        return 0;
    }
    else if (box_left._min.x - box_right._min.x < 0.0f) {
        return -1;
    }
    else {
        return 1;
    }
}

int box_compare_y(const void *a, const void *b) {

    aabb box_left, box_right;
    scene_object *a_obj = *(scene_object**) a;
    scene_object *b_obj = *(scene_object**) b;

    if (!(a_obj->bounding_box(&box_left, 0, 0) && b_obj->bounding_box(&box_right, 0, 0))) {
        OutputDebugStringA("no bounding box in bvh_node constructor\n");
        DebugBreak();
    }

    if (box_left._min.y - box_right._min.y == 0.0f) {
        return 0;
    }
    else if (box_left._min.y - box_right._min.y < 0.0f) {
        return -1;
    }
    else {
        return 1;
    }
}

int box_compare_z(const void *a, const void *b) {

    aabb box_left, box_right;
    scene_object *a_obj = *(scene_object**) a;
    scene_object *b_obj = *(scene_object**) b;

    if (!(a_obj->bounding_box(&box_left, 0, 0) && b_obj->bounding_box(&box_right, 0, 0))) {
        OutputDebugStringA("no bounding box in bvh_node constructor\n");
        DebugBreak();
    }

    if (box_left._min.z - box_right._min.z == 0.0f) {
        return 0;
    }
    else if (box_left._min.z - box_right._min.z < 0.0f) {
        return -1;
    }
    else {
        return 1;
    }
}

typedef int (*cmp_fn)(const void *a, const void *b);

inline cmp_fn box_compare(int axis) {

    switch (axis) {
    case 0:
        return box_compare_x;
    case 1:
        return box_compare_y;
    case 2:
        return box_compare_z;
    default:
        OutputDebugStringA("invalid coordinate axis in box_compare\n");
        DebugBreak();
        return nullptr;
    }
}

bvh_node::bvh_node(scene_object* list[], int n, float time0, float time1) {

    // TODO: build bbox once, then separate into two each recursion without rebuilding the entire thing from scratch (pass new bbox down)

    // create temporary list object to get bounding box of all objects (lazy coder at work!)
    object_list ol(list, n, time0, time1);
    ol.bounding_box(&box, time0, time1);

    // choose split axis by picking largest extents on bb
    int axis = 0;
    vec3 dim = box._max - box._min;
    float max_dim = 0.0f;
    for (int i = 0; i < 3; i++) {
        if (dim[i] > max_dim) {
            max_dim = dim[i];
            axis = i;
        }
    }
    
    qsort(list, n, sizeof(scene_object*), box_compare(axis));

    if (n == 1) {
        left = right = list[0];
    }
    else if (n == 2) {
        left = list[0];
        right = list[1];
    }
    else if (n < 11) {
        left = new object_list(list, n / 2, time0, time1);
        right = new object_list(list + (n / 2), n - (n / 2), time0, time1);
    }
    else {
        left = new bvh_node(list, n / 2, time0, time1);
        right = new bvh_node(list + (n / 2), n - (n / 2), time0, time1);
    }

    //aabb box_left, box_right;
    //if (!(left->bounding_box(&box_left, time0, time1) && right->bounding_box(&box_right, time0, time1))) {
    //    OutputDebugStringA("no bounding box in bvh_node constructor\n");
    //    DebugBreak();
    //}

    //box = surrounding_box(box_left, box_right);
}

/////////////////////////
//     TRANSLATION     //
/////////////////////////

class translate : public scene_object {
public:
    scene_object *obj;
    vec3 offset;

    translate(scene_object *o, const vec3& displacement) : obj(o), offset(displacement) {}
    virtual bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const override {
        ray moved_ray(r.origin - offset, r.dir, r.time);

        if (obj->hit(moved_ray, tmin, tmax, rec)){
            rec->p += offset;
            return true;
        }
        else
            return false;
    }

    virtual bool bounding_box(aabb *box, float time0, float time1) const override {
        if (obj->bounding_box(box, time0, time1)) {
            *box = aabb(box->_min + offset, box->_max + offset);
            return true;
        }
        else
            return false;
    }
};


////////////////////////
//      ROTATION      //
////////////////////////

class rotate_y : public scene_object {
public:
    scene_object *obj;
    aabb bbox;
    float sin_theta;
    float cos_theta;
    bool hasBox;

    rotate_y(scene_object *o, float angle);

    virtual bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const override;
    virtual bool bounding_box(aabb *box, float time0, float time1) const override {
        *box = bbox;
        return hasBox;
    }
};

rotate_y::rotate_y(scene_object *o, float angle) {
    obj = o;
    float radians = (M_PI_F / 180.0f) * angle;
    sin_theta = sin(radians);
    cos_theta = cos(radians);
    hasBox = obj->bounding_box(&bbox, 0, 1);

    if (!hasBox) {
        bbox = aabb(vec3(1, 1, 1), vec3(-1, -1, -1));
        return;
    }

    vec3 minbb(FLT_MAX, FLT_MAX, FLT_MAX);
    vec3 maxbb(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            for (int k = 0; k < 2; k++)
            {
                float x = i * bbox._max.x + (1 - i) * bbox._min.x;
                float y = j * bbox._max.y + (1 - j) * bbox._min.y;
                float z = k * bbox._max.z + (1 - k) * bbox._min.z;

                float newx = cos_theta*x + sin_theta*z;
                float newz = cos_theta*z - sin_theta*x;

                vec3 testvec(newx, y, newz);
                for (int c = 0; c < 3; c++) {
                    if (testvec[c] > maxbb[c])
                        maxbb[c] = testvec[c];
                    if (testvec[c] < minbb[c])
                        minbb[c] = testvec[c];
                }
                
            }
        }
    }
    bbox = aabb(minbb, maxbb);
}

bool rotate_y::hit(const ray& r, float tmin, float tmax, hit_record *rec) const {

    if (hasBox && !bbox.hit(r, tmin, tmax)) // it's faster to test against bbox first before rotating
        return false;

    vec3 origin = r.origin;
    vec3 dir = r.dir;
    origin.x = cos_theta * r.origin.x - sin_theta * r.origin.z;
    origin.z = cos_theta * r.origin.z + sin_theta * r.origin.x;
    dir.x = cos_theta * r.dir.x - sin_theta * r.dir.z;
    dir.z = cos_theta * r.dir.z + sin_theta * r.dir.x;

    ray rotated_ray(origin, dir, r.time);

    if (obj->hit(rotated_ray, tmin, tmax, rec)) {
        vec3 p = rec->p;
        vec3 n = rec->n;
        p.x = cos_theta * rec->p.x + sin_theta * rec->p.z;
        p.z = cos_theta * rec->p.z - sin_theta * rec->p.x;

        n.x = cos_theta * rec->n.x + sin_theta * rec->n.z;
        n.z = cos_theta * rec->n.z - sin_theta * rec->n.x;
        rec->p = p;
        rec->n = n;
        return true;
    }
    else
        return false;
}