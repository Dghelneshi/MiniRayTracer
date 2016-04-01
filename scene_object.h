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
    material *mat_ptr;
};

// abstract base class
class scene_object {
public:
    virtual bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const = 0;
    virtual bool bounding_box(aabb* box, float t0, float t1) const = 0;

    virtual ~scene_object() {}
};


//////////////////////////////////////////////////////////////////////////////////////


class object_list : public scene_object {
public:
    scene_object **list;
    int count;
    aabb box;

    object_list() {}
    object_list(scene_object* l[], int n, float time0, float time1);

    virtual bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const;

    virtual bool bounding_box(aabb* b, float t0, float t1) const {

        if (box._min.x > box._max.x) { // list contains objects with no valid bbox
            return false;
        }
        else {
            *b = box;
            return true;
        }
    }
};

bool object_list::hit(const ray& r, float tmin, float tmax, hit_record *rec) const {
    
    if (box.hit(r, tmin, tmax)) {
        hit_record cur_rec;
        bool hit = false;
        float closest = tmax;

        for (int i = 0; i < count; i++) {

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

object_list::object_list(scene_object* l[], int n, float time0, float time1) {

    list = l;
    count = n;

    vec3 minbb(INFINITY, INFINITY, INFINITY);
    vec3 maxbb(-INFINITY, -INFINITY, -INFINITY);

    for (int i = 0; i < count; i++)
    {
        aabb curbox;
        bool hasBox = list[i]->bounding_box(&curbox, time0, time1);

        if (hasBox) {
            for (int axis = 0; axis < 3; axis++)
            {
                minbb[axis] = min(minbb[axis], curbox._min[axis]);
                maxbb[axis] = max(maxbb[axis], curbox._max[axis]);
            }
        }
        else {
            box = aabb(minbb, maxbb);
            return;
        }
    }
    
    box = aabb(minbb, maxbb);
}


//////////////////////////////////////////////////////////////////////////////////////


class bvh_node : public scene_object {
public:
    scene_object *left;
    scene_object *right;
    aabb box;
    
    bvh_node() {}
    bvh_node(scene_object* list[], int n, float time0, float time1, pcg32_random_t *rng);
    virtual bool bounding_box(aabb* b, float t0, float t1) const {
        *b = box;
        return true;
    }
    virtual bool hit(const ray& r, float tmin, float tmax, hit_record *rec) const;
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

    if (box_left._min.x - box_right._min.x == 0.0f) { // 0/equal case must exist, can otherwise cause infinite loop on msvc!
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

bvh_node::bvh_node(scene_object* list[], int n, float time0, float time1, pcg32_random_t *rng) {

    int axis = int(3 * randf(rng));

    qsort(list, n, sizeof(scene_object*), box_compare(axis));

    if (n == 1) {
        left = right = list[0];
    }
    else if (n == 2) {
        left = list[0];
        right = list[1];
    }
    else {
        left = new bvh_node(list, n / 2, time0, time1, rng);
        right = new bvh_node(list + (n / 2), n - (n / 2), time0, time1, rng);
    }

    aabb box_left, box_right;
    if (!(left->bounding_box(&box_left, time0, time1) && right->bounding_box(&box_right, time0, time1))) {
        OutputDebugStringA("no bounding box in bvh_node constructor\n");
        DebugBreak();
    }

    box = surrounding_box(box_left, box_right);
}
