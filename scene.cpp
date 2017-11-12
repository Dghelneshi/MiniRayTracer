//////////////////////////
//        SCENES        //
//////////////////////////
#include "common.h"
#include "vec3.h"
#include "all_scene_objects.h"
#include "scene.h"
#include "camera.h"
#include "obj_loader.h"
#include "mat4.h"
#include "platform.h"

#include "stb_image.h"

static scene random_scene(int n, float aspect);
static scene random_scene_2(int n, float aspect);
static scene two_spheres(float aspect);
static scene spheres_perlin(float aspect);
static scene earth(float aspect);
static scene cornell_box(float aspect);
static scene cornell_smoke(float aspect);
static scene book2_final(float aspect);
static scene triangles(float aspect);

scene select_scene(scenes choose, float aspect) {
    switch (choose) {
    case SCENE_RANDOM_SPHERES:
        return random_scene(500, aspect);
    case SCENE_RANDOM_SPHERES_2:
        return random_scene_2(500, aspect);
    case SCENE_TWO_SPHERES:
        return two_spheres(aspect);
    case SCENE_PERLIN_SPHERES:
        return spheres_perlin(aspect);
    case SCENE_EARTH:
        return earth(aspect);
    case SCENE_CORNELL_BOX:
        return cornell_box(aspect);
    case SCENE_CORNELL_SMOKE:
        return cornell_smoke(aspect);
    case SCENE_BOOK2_FINAL:
        return book2_final(aspect);
    case SCENE_TRIANGLES:
        return triangles(aspect);
    default:
        MRT_Assert(false);
        return scene();
    }
}

static scene random_scene(int n, float aspect) {

    // setup camera
    Vec3 cam_pos = { 11, 2.2f, 2.5f };
    Vec3 lookat = { 2.8f, 0.5f, 1.2f };
    Vec3 up = { 0, 1, 0 };
    float vfov = 27.0f;
    float aperture = 0.09f;
    float focus_dist = (cam_pos - lookat).length();
    float shutter_t0 = 0.0f;
    float shutter_t1 = 1.0f;

    camera *cam = new camera(cam_pos, lookat, up, vfov, aspect, aperture, focus_dist, shutter_t0, shutter_t1);

    // setup scene objects
    sphere **list = new sphere*[n + 6];

    texture *checker = new checker_tex(new color_tex(Vec3(0.2f, 0.3f, 0.1f)), new color_tex(Vec3(0.9f, 0.9f, 0.9f)), 10.0f);
    list[0] = new sphere(Vec3(0, -1000, 0), 1000, new lambertian(checker));

    int half_sqrt_n = int(sqrtf(float(n)) * 0.5f);
    int i = 1;
    for (int a = -half_sqrt_n; a < half_sqrt_n; a++) {

        for (int b = -half_sqrt_n; b < half_sqrt_n; b++) {

            float choose_mat = randf();
            Vec3 center(a + 0.9f * randf(), 0.2f, b + 0.9f * randf());

            if ((center - Vec3(4, 0.2f, 0)).length() > 0.9f) {

                material *mat;
                sphere *sphere;

                if (choose_mat < 0.5f) {
                    mat = new lambertian(new color_tex(Vec3(randf()*randf(), randf()*randf(), randf()*randf())));
                    sphere = new class sphere(center, 0.2f, mat, center + Vec3 { 0, 0.5f*randf(), 0 }, 0.0f, 1.0f);
                }
                else if (choose_mat < 0.9f) {
                    mat = new metal(new color_tex(0.5f * Vec3(1 + randf(), 1 + randf(), 1 + randf())), randf());
                    sphere = new class sphere(center, 0.2f, mat);
                }
                else {
                    mat = new dielectric(1.4f + randf());
                    sphere = new class sphere(center, 0.2f, mat);
                }

                list[i++] = sphere;
            }
        }
    }

    list[i++] = new sphere(Vec3(0, 1, 0), 1.0f, new dielectric(1.5f));
    list[i++] = new sphere(Vec3(-4, 1, 0), 1.0f, new lambertian(new color_tex(Vec3(0.4f, 0.2f, 0.1f))));
    list[i++] = new sphere(Vec3(4, 1, 0), 1.0f, new metal(new color_tex(Vec3(0.7f, 0.6f, 0.5f)), 1.0f));
    list[i++] = new sphere(Vec3(4, 1, 3), 1.0f, new dielectric(2.4f));
    list[i++] = new sphere(Vec3(4, 1, 3), -0.95f, new dielectric(2.4f));

    // 600x400x16 clang++, 1 thread, 32x32 packets, 16 bounces
    // n:        500 |   1000 | 10000 | 100000         | 1,000,000        
    // list:   62.57 | 137.38 | -- too long ----------------------
    // bvh:    12.45 |  14.14 | 19.04 |  30.79 (+0.35) | 50.32 (+3.45)
    // bvh re:  8.55 |  10.12 | 13.91 |  18.66 (+0.40) | 23.24 (+5.89)

    //scene_object *objects = new object_list<sphere>(list, i, shutter_t0, shutter_t1);
    scene_object *objects = new bvh_node<sphere>(list, i, shutter_t0, shutter_t1);

    return scene { objects, nullptr, cam };
}

static scene random_scene_2(int n, float aspect) {

    // setup camera
    Vec3 cam_pos = { 11, 2.2f, 2.5f };
    Vec3 lookat = { 2.8f, 0.5f, 1.2f };
    Vec3 up = { 0, 1, 0 };
    float vfov = 27.0f;
    float aperture = 0.09f;
    float focus_dist = (cam_pos - lookat).length();
    float shutter_t0 = 0.0f;
    float shutter_t1 = 1.0f;

    camera *cam = new camera(cam_pos, lookat, up, vfov, aspect, aperture, focus_dist, shutter_t0, shutter_t1);

    // setup scene objects
    sphere **list = new sphere*[n + 6];

    int width, height, channels;
    uint8 *pixels = stbi_load("../earthmap.jpg", &width, &height, &channels, 3);
    MRT_Assert(pixels);

    material *earth = new lambertian(new image_tex(pixels, width, height));
    material *checker = new lambertian(new checker_tex(new color_tex(Vec3(0.2f, 0.3f, 0.1f)), new color_tex(Vec3(0.9f)), 10.0f));
    material *perlin = new lambertian(new perlin_tex(1.0f));
    material *perlin_small = new lambertian(new perlin_tex(4.0f));

    list[0] = new sphere(Vec3(0, -1000, 0), 1000, perlin);

    int half_sqrt_n = int(sqrtf(float(n)) * 0.5f);
    int i = 1;
    for (int a = -half_sqrt_n; a < half_sqrt_n; a++) {

        for (int b = -half_sqrt_n; b < half_sqrt_n; b++) {

            float choose_mat = randf();
            Vec3 center(a + 0.9f * randf(), 0.2f, b + 0.9f * randf());

            if ((center - Vec3(4, 0.2f, 0)).length() > 0.9f) {

                material *mat;
                sphere *sphere;

                if (choose_mat < 0.3f) {
                    mat = new lambertian(new color_tex(Vec3(randf()*randf(), randf()*randf(), randf()*randf())));
                    sphere = new class sphere(center, 0.2f, mat, center + Vec3 { 0, 0.5f*randf(), 0 }, 0.0f, 1.0f);
                }
                else {
                    if (choose_mat < 0.6f) {
                        mat = new metal(new color_tex(0.5f * Vec3(1 + randf(), 1 + randf(), 1 + randf())), randf());
                    }
                    else if (choose_mat < 0.7f) {
                        mat = new dielectric(1.4f + randf());
                    }
                    else if (choose_mat < 0.75f) {
                        mat = earth;
                    }
                    else {
                        mat = perlin_small;
                    }
                    sphere = new class sphere(center, 0.2f, mat);
                }

                list[i++] = sphere;
            }
        }
    }


    list[i++] = new sphere(Vec3(0, 1, 0), 1.0f, new dielectric(1.5f));
    list[i++] = new sphere(Vec3(-4, 1, 0), 1.0f, checker);
    list[i++] = new sphere(Vec3(4, 1, 0), 1.0f, new metal(new color_tex(Vec3(0.7f, 0.6f, 0.5f)), 1.0f));
    list[i++] = new sphere(Vec3(4, 1, 3), 1.0f, new dielectric(2.4f));
    list[i++] = new sphere(Vec3(4, 1, 3), -0.95f, new dielectric(2.4f));

    scene_object *objects = new bvh_node<sphere>(list, i, shutter_t0, shutter_t1);

    return scene { objects, nullptr, cam };
}


static scene two_spheres(float aspect) {

    // setup camera
    Vec3 cam_pos = { 11, 2.2f, 2.5f };
    Vec3 lookat = { 2.8f, 0.5f, 1.2f };
    Vec3 up = { 0, 1, 0 };
    float vfov = 27.0f;
    float aperture = 0.09f;
    float focus_dist = (cam_pos - lookat).length();
    float shutter_t0 = 0.0f;
    float shutter_t1 = 1.0f;

    camera *cam = new camera(cam_pos, lookat, up, vfov, aspect, aperture, focus_dist, shutter_t0, shutter_t1);

    // setup scene objects
    texture *checker = new checker_tex(new color_tex(Vec3(0.2f, 0.3f, 0.1f)), new color_tex(Vec3(0.9f)), 10.0f);

    sphere **list = new sphere*[2];
    list[0] = new sphere(Vec3(0, -10, 0), 10, new lambertian(checker));
    list[1] = new sphere(Vec3(0, 10, 0), 10, new lambertian(checker));

    scene_object *objects = new object_list<sphere>(list, 2, shutter_t0, shutter_t1);

    return scene { objects, nullptr, cam };
}

static scene spheres_perlin(float aspect) {

    // setup camera
    Vec3 cam_pos = { 11, 2.2f, 2.5f };
    Vec3 lookat = { 2.8f, 0.5f, 1.2f };
    Vec3 up = { 0, 1, 0 };
    float vfov = 27.0f;
    float aperture = 0.09f;
    float focus_dist = (cam_pos - lookat).length();
    float shutter_t0 = 0.0f;
    float shutter_t1 = 1.0f;

    camera *cam = new camera(cam_pos, lookat, up, vfov, aspect, aperture, focus_dist, shutter_t0, shutter_t1);

    // setup scene objects
    sphere **list = new sphere*[3];
    list[0] = new sphere(Vec3(0, -1001, 0), 1000, new lambertian(new perlin_tex(1.0f)));
    list[1] = new sphere(Vec3(0, 1, 0), 2, new lambertian(new perlin_tex(4.0f)));
    list[2] = new sphere(Vec3(0.5f, -0.5f, 2), 0.5f, new lambertian(new perlin_tex(16.0f)));

    scene_object *objects = new object_list<sphere>(list, 3, shutter_t0, shutter_t1);

    return scene { objects, nullptr, cam };
}

static scene earth(float aspect) {

    // setup camera
    Vec3 cam_pos = { 11, 2.2f, 2.5f };
    Vec3 lookat = { 2.8f, 0.5f, 1.2f };
    Vec3 up = { 0, 1, 0 };
    float vfov = 27.0f;
    float aperture = 0.09f;
    float focus_dist = (cam_pos - lookat).length();
    float shutter_t0 = 0.0f;
    float shutter_t1 = 1.0f;

    camera *cam = new camera(cam_pos, lookat, up, vfov, aspect, aperture, focus_dist, shutter_t0, shutter_t1);

    // setup scene objects
    int width, height, channels;
    uint8 *pixels = stbi_load("../earthmap.jpg", &width, &height, &channels, 3);
    MRT_Assert(pixels);

    material *mat = new lambertian(new image_tex(pixels, width, height));

    sphere **list = new sphere*[3];
    list[0] = new sphere(Vec3(0, -1001, 0), 1000, new lambertian(new perlin_tex(1.0f)));
    list[1] = new sphere(Vec3(0, 1, 0), 2, mat);
    list[2] = new sphere(Vec3(0.5f, -0.5f, 2), 0.5f, mat);

    scene_object *objects = new object_list<sphere>(list, 3, shutter_t0, shutter_t1);

    return scene { objects, nullptr, cam };
}

static scene cornell_box(float aspect) {

    // setup camera
    Vec3 cam_pos = { 278, 278, -800 };
    Vec3 lookat = { 278, 278, 100 };
    Vec3 up = { 0, 1, 0 };
    float vfov = 40.0f;
    float aperture = 0.00f;
    float focus_dist = (cam_pos - lookat).length();
    float shutter_t0 = 0.0f;
    float shutter_t1 = 1.0f;

    camera *cam = new camera(cam_pos, lookat, up, vfov, aspect, aperture, focus_dist, shutter_t0, shutter_t1);

    // setup scene objects
    int n = 8;
    scene_object **list = new scene_object*[n];
    int i = 0;

    material *red = new lambertian(new color_tex(Vec3(0.65f, 0.055f, 0.06f)));
    material *white = new lambertian(new color_tex(Vec3(0.73f, 0.73f, 0.73f)));
    material *green = new lambertian(new color_tex(Vec3(0.117f, 0.44f, 0.115f)));
    //material *light = new diffuse_light(new color_tex(Vec3(16.86f, 10.76f, 3.7f)));
    material *light = new diffuse_light(new color_tex(Vec3(15.f)));

    //material *aluminum = new metal(new color_tex(Vec3(0.8f, 0.85f, 0.88f)), 1.0f);
    material *glass = new dielectric(1.5f);

    list[i++] = new yz_rect(555, 0, 0, 555, 555, green);
    list[i++] = new yz_rect(0, 555, 0, 555, 0, red);
    xz_rect *l = new xz_rect(343, 213, 227, 332, 554, light);
    list[i++] = l;
    //list[i++] = new xz_rect(443, 113, 127, 432, 554, light);
    list[i++] = new xz_rect(555, 0, 0, 555, 555, white);
    list[i++] = new xz_rect(0, 555, 0, 555, 0, white);
    list[i++] = new xy_rect(555, 0, 0, 555, 555, white);
    list[i++] = new translate(new rotate_y(new box(Vec3(0, 0, 0), Vec3(165, 330, 165), white), 15), Vec3(265, 0, 295));
    //list[i++] = new translate(new rotate_y(new box(Vec3(0, 0, 0), Vec3(165, 165, 165), white), -18), Vec3(130, 0, 65));
    sphere *s = new sphere(Vec3(190, 90, 190), 90, glass);
    list[i++] = s;

    scene_object *objects = new object_list<scene_object>(list, i, shutter_t0, shutter_t1);

    scene_object **b = new scene_object*[2];
    b[0] = l;
    b[1] = s;
    scene_object *biased = new object_list<scene_object>(b, 1, shutter_t0, shutter_t1);

    return scene { objects, biased, cam };
}

static scene cornell_smoke(float aspect) {

    // setup camera
    Vec3 cam_pos = { 278, 278, -800 };
    Vec3 lookat = { 278, 278, 100 };
    Vec3 up = { 0, 1, 0 };
    float vfov = 40.0f;
    float aperture = 0.00f;
    float focus_dist = (cam_pos - lookat).length();
    float shutter_t0 = 0.0f;
    float shutter_t1 = 1.0f;

    camera *cam = new camera(cam_pos, lookat, up, vfov, aspect, aperture, focus_dist, shutter_t0, shutter_t1);

    // setup scene objects
    int n = 8;
    scene_object **list = new scene_object*[n];
    int i = 0;

    material *red = new lambertian(new color_tex(Vec3(0.65f, 0.05f, 0.05f)));
    material *white = new lambertian(new color_tex(Vec3(0.73f, 0.73f, 0.73f)));
    material *green = new lambertian(new color_tex(Vec3(0.12f, 0.45f, 0.15f)));
    material *light = new diffuse_light(new color_tex(Vec3(7.0f)));

    list[i++] = new yz_rect(555, 0, 0, 555, 555, green);
    list[i++] = new yz_rect(0, 555, 0, 555, 0, red);
    //list[i++] = new xz_rect(343, 213, 227, 332, 554, light); // smaller light, needs A LOT more samples without bias
    xz_rect *l = new xz_rect(443, 113, 127, 432, 554, light);
    list[i++] = l;
    list[i++] = new xz_rect(555, 0, 0, 555, 555, white);
    list[i++] = new xz_rect(0, 555, 0, 555, 0, white);
    list[i++] = new xy_rect(555, 0, 0, 555, 555, white);
    scene_object *smoke_box1 = new translate(new rotate_y(new box(Vec3(0, 0, 0), Vec3(165, 165, 165), white), -18), Vec3(130, 0, 65));
    scene_object *smoke_box2 = new translate(new rotate_y(new box(Vec3(0, 0, 0), Vec3(165, 330, 165), white), 15), Vec3(265, 0, 295));
    list[i++] = new constant_volume(smoke_box1, 0.01f, new color_tex(Vec3(1.0f, 1.0f, 1.0f)));
    list[i++] = new constant_volume(smoke_box2, 0.01f, new color_tex(Vec3(0.0f, 0.0f, 0.0f)));

    scene_object *objects = new object_list<scene_object>(list, n, shutter_t0, shutter_t1);

    scene_object **b = new scene_object*[1];
    b[0] = l;
    scene_object *biased = new object_list<scene_object>(b, 1, shutter_t0, shutter_t1);

    return scene { objects, biased, cam };
}

static scene book2_final(float aspect) {

    // setup camera
    Vec3 cam_pos = { 450, 278, -560 };
    Vec3 lookat = { 200, 278, 300 };
    Vec3 up = { 0, 1, 0 };
    float vfov = 40.0f;
    float aperture = 0.00f;
    float focus_dist = (cam_pos - lookat).length();
    float shutter_t0 = 0.0f;
    float shutter_t1 = 1.0f;

    camera *cam = new camera(cam_pos, lookat, up, vfov, aspect, aperture, focus_dist, shutter_t0, shutter_t1);

    // setup scene objects
    int nb = 20;
    int ns = 1000;
    scene_object **list = new scene_object*[30];
    box **boxlist = new box*[nb*nb];
    sphere **spherelist = new sphere*[ns];

    int width, height, channels;
    uint8 *pixels = stbi_load("../earthmap.jpg", &width, &height, &channels, 3);
    MRT_Assert(pixels);

    material *earth = new lambertian(new image_tex(pixels, width, height));
    material *white = new lambertian(new color_tex(Vec3(0.73f, 0.73f, 0.73f)));
    material *green = new lambertian(new color_tex(Vec3(0.48f, 0.83f, 0.53f)));
    material *light = new diffuse_light(new color_tex(Vec3(7.0f)));
    material *orange = new lambertian(new color_tex(Vec3(0.7f, 0.3f, 0.1f)));
    material *perlin = new lambertian(new perlin_tex(0.05f));


    // green boxes
    int b = 0;
    for (int i = 0; i < nb; i++) {
        for (int j = 0; j < nb; j++) {
            float w = 100;
            float x0 = -1000 + i*w;
            float z0 = -1000 + j*w;
            float y0 = 0;
            float x1 = x0 + w;
            float y1 = 100 * (randf() + 0.01f);
            float z1 = z0 + w;
            boxlist[b++] = new box(Vec3(x0, y0, z0), Vec3(x1, y1, z1), green);
        }
    }

    int l = 0;
    list[l++] = new bvh_node<box>(boxlist, b, shutter_t0, shutter_t1); // green boxes
    xz_rect *lo = new xz_rect(423, 123, 147, 412, 554, light); // light
    list[l++] = lo;
    Vec3 center(400, 400, 200);
    list[l++] = new sphere(center, 50, orange, center + Vec3(30, 0, 0), 0, 1);            // orange-brownish sphere
    sphere *gs = new sphere(Vec3(260, 150, 45), 50, new dielectric(1.5f));                 // glass sphere
    list[l++] = gs;
    list[l++] = new sphere(Vec3(0, 150, 145), 50, new metal(new color_tex(Vec3(0.8f, 0.8f, 0.9f)), 0.1f));  // silver sphere
    list[l++] = new sphere(Vec3(400, 200, 400), 100, earth);                              // earth sphere
    list[l++] = new sphere(Vec3(220, 280, 300), 80, perlin);                              // perlin sphere


    scene_object *volume_boundary = new sphere(Vec3(360, 150, 145), 70, new dielectric(1.5f));
    list[l++] = volume_boundary;
    list[l++] = new constant_volume(volume_boundary, 0.2f, new color_tex(Vec3(0.2f, 0.4f, 0.9f))); // blue sphere

    volume_boundary = new sphere(Vec3(0, 0, 0), 5000, new dielectric(1.5));
    list[l++] = new constant_volume(volume_boundary, 0.0001f, new color_tex(Vec3(1.0f))); // fog

                                                                                          // box of white spheres
    for (int i = 0; i < ns; i++) {
        spherelist[i] = new sphere(Vec3(165 * randf(), 165 * randf(), 165 * randf()), 10, white);
    }
    list[l++] = new translate(new rotate_y(new bvh_node<sphere>(spherelist, ns, shutter_t0, shutter_t1), 15), Vec3(-100, 270, 395));

    scene_object *objects = new object_list<scene_object>(list, l, shutter_t0, shutter_t1);

    scene_object **ba = new scene_object*[2];
    ba[0] = lo;
    ba[1] = gs;
    scene_object *biased = new object_list<scene_object>(ba, 1, shutter_t0, shutter_t1);

    return scene { objects, biased, cam };
}

static scene triangles(float aspect) {

    // setup camera
    Vec3 cam_pos = { 278, 278, -800 };
    Vec3 lookat = { 278, 278, 100 };
    Vec3 up = { 0, 1, 0 };
    float vfov = 40.0f;
    float aperture = 20.0f;
    float focus_dist = (cam_pos - lookat).length();
    float shutter_t0 = 0.0f;
    float shutter_t1 = 1.0f;

    camera *cam = new camera(cam_pos, lookat, up, vfov, aspect, aperture, focus_dist, shutter_t0, shutter_t1);

    // setup scene objects
    size_t n = 10;
    scene_object **list = new scene_object*[n];
    size_t i = 0;

    material *red = new lambertian(new color_tex(Vec3(0.65f, 0.05f, 0.05f)));
    material *white = new lambertian(new color_tex(Vec3(0.73f, 0.73f, 0.73f)));
    material *green = new lambertian(new color_tex(Vec3(0.12f, 0.45f, 0.15f)));
    material *light = new diffuse_light(new color_tex(Vec3(4.0f)));
    material *silver = new metal(new color_tex(Vec3(0.8f, 0.8f, 0.9f)), 0.9f);
    material *dia = new dielectric(2.4f);

    list[i++] = new yz_rect(555, 0, 0, 555, 555, green);
    list[i++] = new yz_rect(0, 555, 0, 555, 0, red);
    //xz_rect *l = new xz_rect(343, 213, 227, 332, 554, light); // smaller light, needs A LOT more samples without bias
    xz_rect *l = new xz_rect(443, 113, 127, 432, 554, light);
    list[i++] = l;
    list[i++] = new xz_rect(555, 0, 0, 555, 555, white);
    list[i++] = new xz_rect(0, 555, 0, 555, 0, white);
    list[i++] = new xy_rect(555, 0, 0, 555, 555, silver);

    // list[i++] = new translate(new triangle(2 * Vec3(0, 0, 82.5f), 2 * Vec3(82.5f, 82.5f, 120), 2 * Vec3(185, 0, 0), silver), Vec3(90, 0, 165));
    // list[i++] = new translate(new triangle(2 * Vec3(0, 0, 0), 2 * Vec3(82.5f, 82.5f, 100), 2 * Vec3(165, 0, 82.5f), silver), Vec3(185, 0, 95));

    size_t tris = 0;
    triangle **bunny = readObj("../obj/bunny.obj", dia, &tris, true, Mat4::Scale(2000.0f), Vec3(195, -20, 280));
    if (tris && bunny) {
        list[i++] = new bvh_node<triangle>(bunny, tris, shutter_t0, shutter_t1);
    }

    tris = 0;
    triangle **teapot = readObj("../obj/teapot3_no_vt.obj", dia, &tris, false, Mat4::Scale(250.0f), Vec3(393, 50, 108), Mat4::RotateY(RAD(30)));
    if (tris && teapot) {
        ////list[i++] = new rotate_y(new bvh_node(teapot, tris, shutter_t0, shutter_t1), 30);
        list[i++] = new bvh_node<triangle>(teapot, tris, shutter_t0, shutter_t1);
    }

    /*   tris = 0;
    triangle **spider = readObj("../obj/spider_pruned.obj", dia, &tris, false, 1.3f, Vec3(385, 70, 100));
    if (tris && spider) {
    list[i++] = new bvh_node<triangle>(spider, tris, shutter_t0, shutter_t1);
    }
    */
    scene_object *objects = new object_list<scene_object>(list, i, shutter_t0, shutter_t1);

    // TODO: use bounding box to generate pdfs for bvh
    scene_object **ba = new scene_object*[1];
    ba[0] = l;
    scene_object *biased = new object_list<scene_object>(ba, 1, shutter_t0, shutter_t1);

    return scene { objects, biased, cam };
}
