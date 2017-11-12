#pragma once

#include "camera.h"
#include "scene_object.h"

enum scenes : uint32 {
    SCENE_RANDOM_SPHERES,
    SCENE_RANDOM_SPHERES_2,
    SCENE_TWO_SPHERES,
    SCENE_PERLIN_SPHERES,
    SCENE_EARTH,
    SCENE_CORNELL_BOX,
    SCENE_CORNELL_SMOKE,
    SCENE_BOOK2_FINAL,
    SCENE_TRIANGLES,
    ENUM_SCENES_MAX
};

struct scene {
    scene_object *objects;
    scene_object *biased_objects;
    camera *camera;
};

scene select_scene(scenes choose, float aspect);
