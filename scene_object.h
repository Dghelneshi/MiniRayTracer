#pragma once

#include "ray.h"
#include "aabb.h"
#include "pcg.h"
#include <stdlib.h> // qsort

class material;

struct hit_record {
    float t;
    Vec3 p;
    Vec3 n;
    float u;
    float v;
    material *mat_ptr;
};


class scene_object {
public:
    virtual bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const = 0;
    virtual bool bounding_box(aabb* box, float time0, float time1) const = 0;
    virtual float pdf_value(const Vec3& origin, const Vec3& dir, float time) const {
        return 0;
    }
    virtual Vec3 pdf_generate(const Vec3& origin, float time) const {
        return Vec3(1, 0, 0);
    }

    virtual ~scene_object() {}
};

//////////////////////////////////////////////////////////////////////////////////////

// TODO: test invalid bbox code (for objects like planes)

template <typename T>
class object_list final : public scene_object {
public:
    T **list;
    size_t count;
    aabb box;
    bool hasBox;

    object_list(T* l[], size_t n, float time0, float time1);

    bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const override;
    bool bounding_box(aabb* b, float time0, float time1) const override;
    float pdf_value(const Vec3& origin, const Vec3& dir, float time) const override;
    Vec3 pdf_generate(const Vec3& origin, float time) const override;
};

template <typename T>
bool object_list<T>::bounding_box(aabb* b, float time0, float time1) const {
    if (!hasBox) {
        return false;
    }
    else {
        *b = box;
        return true;
    }
}

template <typename T>
float object_list<T>::pdf_value(const Vec3& origin, const Vec3& dir, float time) const {
    float sum = 0;
    for (size_t i = 0; i < count; i++) {
        sum += list[i]->pdf_value(origin, dir, time);
    }
    return sum / count;
}

template <typename T>
Vec3 object_list<T>::pdf_generate(const Vec3& origin, float time) const {
    int i = int(randf() * count);
    return list[i]->pdf_generate(origin, time);
}

template <typename T>
bool object_list<T>::hit(const ray& r, float tmin, float tmax, hit_record *rec) const {

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

template <typename T>
object_list<T>::object_list(T* l[], size_t n, float time0, float time1) {

    list = l;
    count = n;

    Vec3 minbb(std::numeric_limits<float>::max(),    std::numeric_limits<float>::max(),    std::numeric_limits<float>::max());
    Vec3 maxbb(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

    for (size_t i = 0; i < count; i++)
    {
        aabb curbox;
        bool cur_hasBox = list[i]->bounding_box(&curbox, time0, time1);

        if (cur_hasBox) {
            minbb = vmin(minbb, curbox.min);
            maxbb = vmax(maxbb, curbox.max);
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

template <typename T>
class bvh_node final : public scene_object {
public:
    scene_object *left;
    scene_object *right;
    aabb box;
    
    bvh_node(T* list[], size_t n, float time0, float time1);

    bool bounding_box(aabb* b, float time0, float time1) const override {
        *b = box;
        return true;
    }
    bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const override;
};

template <typename T>
bool bvh_node<T>::hit(const ray& r, float tmin, float tmax, hit_record *rec) const {

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

template <size_t axis, typename T>
inline int __cdecl box_compare(const void *a, const void *b) {

    aabb box_left, box_right;
    T *a_obj = *(T**) a;
    T *b_obj = *(T**) b;

    MRT_Assert(a_obj->bounding_box(&box_left, 0, 0) && b_obj->bounding_box(&box_right, 0, 0),
               "no bounding box in bvh_node constructor\n");

    float delta = box_left.min[axis] - box_right.min[axis];

    if (delta == 0.0f) { // MSVC explicitly requires this case for qsort, otherwise infinite loop can occur
        return 0;
    }
    else if (delta < 0.0f) {
        return -1;
    }
    else {
        return 1;
    }
}

typedef int(__cdecl *cmp_fn)(const void *a, const void *b);

template <typename T>
inline cmp_fn box_compare_dispatch(size_t axis) {
    switch (axis) {
    case 0: return box_compare<0, T>;
    case 1: return box_compare<1, T>;
    case 2: return box_compare<2, T>;
    default:
        return nullptr;
    }
}

template <typename T>
bvh_node<T>::bvh_node(T* list[], size_t n, float time0, float time1) {

    // TODO: build bbox once, then separate into two each recursion without rebuilding the entire thing from scratch (pass new bbox down)

    // create temporary list object to get bounding box of all objects (lazy coder at work!)
    object_list<T> ol(list, n, time0, time1);
    ol.bounding_box(&box, time0, time1);

    // choose split axis by picking largest extents on bb
    Vec3 dim = box.max - box.min;
    size_t axis = max_dim(dim);

    qsort(list, n, sizeof(T*), box_compare_dispatch<T>(axis));

    if (n == 1) {
        left = right = list[0];
    }
    else if (n == 2) {
        left = list[0];
        right = list[1];
    }
    else if (n < 11) {
        left = new object_list<T>(list, n / 2, time0, time1);
        right = new object_list<T>(list + (n / 2), n - (n / 2), time0, time1);
    }
    else {
        left = new bvh_node(list, n / 2, time0, time1);
        right = new bvh_node(list + (n / 2), n - (n / 2), time0, time1);
    }

    //aabb box_left, box_right;
    //MRT_Assert(left->bounding_box(&box_left, time0, time1) && right->bounding_box(&box_right, time0, time1),
    //           "no bounding box in bvh_node constructor\n");
    //box = surrounding_box(box_left, box_right);
}

/////////////////////////
//     TRANSLATION     //
/////////////////////////

class translate final : public scene_object {
public:
    scene_object *obj;
    Vec3 offset;

    translate(scene_object *o, const Vec3& displacement) : obj(o), offset(displacement) {}
    bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const override;
    bool bounding_box(aabb *box, float time0, float time1) const override;
};


////////////////////////
//      ROTATION      //
////////////////////////

class rotate_y final : public scene_object {
public:
    scene_object *obj;
    aabb bbox;
    float sin_theta;
    float cos_theta;
    bool hasBox;

    rotate_y(scene_object *o, float angle);

    bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const override;
    bool bounding_box(aabb *box, float time0, float time1) const override {
        *box = bbox;
        return hasBox;
    }
};