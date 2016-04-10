//////////////////////////
//        SCENES        //
//////////////////////////

enum scenes {
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
    camera *camera;
};

scene random_scene(int n);
scene random_scene_2(int n);
scene two_spheres();
scene spheres_perlin();
scene earth();
scene cornell_box();
scene cornell_smoke();
scene book2_final();
scene triangles();

scene select_scene(scenes choose) {
    switch (choose) {
    case SCENE_RANDOM_SPHERES:
        return random_scene(500);
    case SCENE_RANDOM_SPHERES_2:
        return random_scene_2(500);
    case SCENE_TWO_SPHERES:
        return two_spheres();
    case SCENE_PERLIN_SPHERES:
        return spheres_perlin();
    case SCENE_EARTH:
        return earth();
    case SCENE_CORNELL_BOX:
        return cornell_box();
    case SCENE_CORNELL_SMOKE:
        return cornell_smoke();
    case SCENE_BOOK2_FINAL:
        return book2_final();
    case SCENE_TRIANGLES:
        return triangles();
    default:
        DebugBreak();
        return scene();
    }
}

scene random_scene(int n) {

    // setup camera
    vec3 cam_pos = { 11, 2.2f, 2.5f };
    vec3 lookat = { 2.8f, 0.5f, 1.2f };
    vec3 up = { 0, 1, 0 };
    float vfov = 27.0f;
    float aspect = G_bufferWidth / (float) G_bufferHeight;
    float aperture = 0.09f;
    float focus_dist = (cam_pos - lookat).length();
    float shutter_t0 = 0.0f;
    float shutter_t1 = 1.0f;

    camera *cam = new camera(cam_pos, lookat, up, vfov, aspect, aperture, focus_dist, shutter_t0, shutter_t1);

    // setup scene objects
    scene_object **list = new scene_object*[n + 6];

    texture *checker = new checker_tex(new uni_color_tex(vec3(0.2f, 0.3f, 0.1f)), new uni_color_tex(vec3(0.9f, 0.9f, 0.9f)), 10.0f);
    list[0] = new sphere(vec3(0, -1000, 0), 1000, new lambertian(checker));

    int half_sqrt_n = int(sqrtf(float(n)) * 0.5f);
    int i = 1;
    for (int a = -half_sqrt_n; a < half_sqrt_n; a++) {

        for (int b = -half_sqrt_n; b < half_sqrt_n; b++) {

            float choose_mat = randf();
            vec3 center(a + 0.9f * randf(), 0.2f, b + 0.9f * randf());

            if ((center - vec3(4, 0.2f, 0)).length() > 0.9f) {

                material *mat;
                sphere *sphere;

                if (choose_mat < 0.5f) {
                    mat = new lambertian(new uni_color_tex(vec3(randf()*randf(), randf()*randf(), randf()*randf())));
                    sphere = new class sphere(center, 0.2f, mat, center + vec3{ 0, 0.5f*randf(), 0 }, 0.0f, 1.0f);
                }
                else if (choose_mat < 0.9f) {
                    mat = new metal(0.5f * vec3(1 + randf(), 1 + randf(), 1 + randf()), randf());
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

    list[i++] = new sphere(vec3(0, 1, 0), 1.0f, new dielectric(1.5f));
    list[i++] = new sphere(vec3(-4, 1, 0), 1.0f, new lambertian(new uni_color_tex(vec3(0.4f, 0.2f, 0.1f))));
    list[i++] = new sphere(vec3(4, 1, 0), 1.0f, new metal(vec3(0.7f, 0.6f, 0.5f), 1.0f));
    list[i++] = new sphere(vec3(4, 1, 3), 1.0f, new dielectric(2.4f));
    list[i++] = new sphere(vec3(4, 1, 3), -0.95f, new dielectric(2.4f));

    // 600x400x16 clang++, 1 thread, 32x32 packets, 16 bounces
    // n:        500 |   1000 | 10000 | 100000         | 1,000,000        
    // list:   62.57 | 137.38 | -- too long ----------------------
    // bvh:    12.45 |  14.14 | 19.04 |  30.79 (+0.35) | 50.32 (+3.45)
    // bvh re:  8.55 |  10.12 | 13.91 |  18.66 (+0.40) | 23.24 (+5.89)

    //scene_object *objects = new object_list(list, i, shutter_t0, shutter_t1);
    scene_object *objects = new bvh_node(list, i, shutter_t0, shutter_t1);

    return scene{ objects, cam };
}

scene random_scene_2(int n) {

    // setup camera
    vec3 cam_pos = { 11, 2.2f, 2.5f };
    vec3 lookat = { 2.8f, 0.5f, 1.2f };
    vec3 up = { 0, 1, 0 };
    float vfov = 27.0f;
    float aspect = G_bufferWidth / (float) G_bufferHeight;
    float aperture = 0.09f;
    float focus_dist = (cam_pos - lookat).length();
    float shutter_t0 = 0.0f;
    float shutter_t1 = 1.0f;

    camera *cam = new camera(cam_pos, lookat, up, vfov, aspect, aperture, focus_dist, shutter_t0, shutter_t1);

    // setup scene objects
    scene_object **list = new scene_object*[n + 6];

    int width, height, channels;
    uint8 *pixels = stbi_load("../earthmap.jpg", &width, &height, &channels, 3);
    if (!pixels) DebugBreak();

    material *earth = new lambertian(new image_tex(pixels, width, height));
    material *checker = new lambertian(new checker_tex(new uni_color_tex(vec3(0.2f, 0.3f, 0.1f)), new uni_color_tex(vec3(0.9f, 0.9f, 0.9f)), 10.0f));
    material *perlin = new lambertian(new perlin_tex(1.0f));
    material *perlin_small = new lambertian(new perlin_tex(4.0f));

    list[0] = new sphere(vec3(0, -1000, 0), 1000, perlin);

    int half_sqrt_n = int(sqrtf(float(n)) * 0.5f);
    int i = 1;
    for (int a = -half_sqrt_n; a < half_sqrt_n; a++) {

        for (int b = -half_sqrt_n; b < half_sqrt_n; b++) {

            float choose_mat = randf();
            vec3 center(a + 0.9f * randf(), 0.2f, b + 0.9f * randf());

            if ((center - vec3(4, 0.2f, 0)).length() > 0.9f) {

                material *mat;
                sphere *sphere;

                if (choose_mat < 0.3f) {
                    mat = new lambertian(new uni_color_tex(vec3(randf()*randf(), randf()*randf(), randf()*randf())));
                    sphere = new class sphere(center, 0.2f, mat, center + vec3{ 0, 0.5f*randf(), 0 }, 0.0f, 1.0f);
                }
                else {
                    if (choose_mat < 0.6f) {
                        mat = new metal(0.5f * vec3(1 + randf(), 1 + randf(), 1 + randf()), randf());
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


    list[i++] = new sphere(vec3(0, 1, 0), 1.0f, new dielectric(1.5f));
    list[i++] = new sphere(vec3(-4, 1, 0), 1.0f, checker);
    list[i++] = new sphere(vec3(4, 1, 0), 1.0f, new metal(vec3(0.7f, 0.6f, 0.5f), 1.0f));
    list[i++] = new sphere(vec3(4, 1, 3), 1.0f, new dielectric(2.4f));
    list[i++] = new sphere(vec3(4, 1, 3), -0.95f, new dielectric(2.4f));

    scene_object *objects = new bvh_node(list, i, shutter_t0, shutter_t1);

    return scene{ objects, cam };
}


scene two_spheres() {

    // setup camera
    vec3 cam_pos = { 11, 2.2f, 2.5f };
    vec3 lookat = { 2.8f, 0.5f, 1.2f };
    vec3 up = { 0, 1, 0 };
    float vfov = 27.0f;
    float aspect = G_bufferWidth / (float) G_bufferHeight;
    float aperture = 0.09f;
    float focus_dist = (cam_pos - lookat).length();
    float shutter_t0 = 0.0f;
    float shutter_t1 = 1.0f;

    camera *cam = new camera(cam_pos, lookat, up, vfov, aspect, aperture, focus_dist, shutter_t0, shutter_t1);

    // setup scene objects
    texture *checker = new checker_tex(new uni_color_tex(vec3(0.2f, 0.3f, 0.1f)), new uni_color_tex(vec3(0.9f, 0.9f, 0.9f)), 10.0f);

    scene_object **list = new scene_object*[2];
    list[0] = new sphere(vec3(0, -10, 0), 10, new lambertian(checker));
    list[1] = new sphere(vec3(0, 10, 0), 10, new lambertian(checker));

    scene_object *objects = new object_list(list, 2, shutter_t0, shutter_t1);

    return scene{ objects, cam };
}

scene spheres_perlin() {

    // setup camera
    vec3 cam_pos = { 11, 2.2f, 2.5f };
    vec3 lookat = { 2.8f, 0.5f, 1.2f };
    vec3 up = { 0, 1, 0 };
    float vfov = 27.0f;
    float aspect = G_bufferWidth / (float) G_bufferHeight;
    float aperture = 0.09f;
    float focus_dist = (cam_pos - lookat).length();
    float shutter_t0 = 0.0f;
    float shutter_t1 = 1.0f;

    camera *cam = new camera(cam_pos, lookat, up, vfov, aspect, aperture, focus_dist, shutter_t0, shutter_t1);

    // setup scene objects
    scene_object **list = new scene_object*[3];
    list[0] = new sphere(vec3(0, -1001, 0), 1000, new lambertian(new perlin_tex(1.0f)));
    list[1] = new sphere(vec3(0, 1, 0), 2, new lambertian(new perlin_tex(4.0f)));
    list[2] = new sphere(vec3(0.5f, -0.5f, 2), 0.5f, new lambertian(new perlin_tex(16.0f)));

    scene_object *objects = new object_list(list, 3, shutter_t0, shutter_t1);

    return scene{ objects, cam };
}

scene earth() {

    // setup camera
    vec3 cam_pos = { 11, 2.2f, 2.5f };
    vec3 lookat = { 2.8f, 0.5f, 1.2f };
    vec3 up = { 0, 1, 0 };
    float vfov = 27.0f;
    float aspect = G_bufferWidth / (float) G_bufferHeight;
    float aperture = 0.09f;
    float focus_dist = (cam_pos - lookat).length();
    float shutter_t0 = 0.0f;
    float shutter_t1 = 1.0f;

    camera *cam = new camera(cam_pos, lookat, up, vfov, aspect, aperture, focus_dist, shutter_t0, shutter_t1);

    // setup scene objects
    int width, height, channels;
    uint8 *pixels = stbi_load("../earthmap.jpg", &width, &height, &channels, 3);
    if (!pixels) DebugBreak();

    material *mat = new lambertian(new image_tex(pixels, width, height));

    scene_object **list = new scene_object*[3];
    list[0] = new sphere(vec3(0, -1001, 0), 1000, new lambertian(new perlin_tex(1.0f)));
    list[1] = new sphere(vec3(0, 1, 0), 2, mat);
    list[2] = new sphere(vec3(0.5f, -0.5f, 2), 0.5f, mat);

    scene_object *objects = new object_list(list, 3, shutter_t0, shutter_t1);

    return scene{ objects, cam };
}

scene cornell_box() {

    // setup camera
    vec3 cam_pos = { 278, 278, -800 };
    vec3 lookat = { 278, 278, 100 };
    vec3 up = { 0, 1, 0 };
    float vfov = 40.0f;
    float aspect = G_bufferWidth / (float) G_bufferHeight;
    float aperture = 0.00f;
    float focus_dist = (cam_pos - lookat).length();
    float shutter_t0 = 0.0f;
    float shutter_t1 = 1.0f;

    camera *cam = new camera(cam_pos, lookat, up, vfov, aspect, aperture, focus_dist, shutter_t0, shutter_t1);

    // setup scene objects
    int n = 8;
    scene_object **list = new scene_object*[n];
    int i = 0;

    material *red = new lambertian(new uni_color_tex(vec3(0.65f, 0.05f, 0.05f)));
    material *white = new lambertian(new uni_color_tex(vec3(0.73f, 0.73f, 0.73f)));
    material *green = new lambertian(new uni_color_tex(vec3(0.12f, 0.45f, 0.15f)));
    material *light = new diffuse_light(new uni_color_tex(vec3(5.0f, 5.0f, 5.0f)));

    list[i++] = new yz_rect(555, 0, 0, 555, 555, green);
    list[i++] = new yz_rect(0, 555, 0, 555, 0, red);
    //list[i++] = new xz_rect(343, 213, 227, 332, 554, light); // smaller light, needs A LOT more samples without bias
    list[i++] = new xz_rect(443, 113, 127, 432, 554, light);
    list[i++] = new xz_rect(555, 0, 0, 555, 555, white);
    list[i++] = new xz_rect(0, 555, 0, 555, 0, white);
    list[i++] = new xy_rect(555, 0, 0, 555, 555, white);
    list[i++] = new translate(new rotate_y(new box(vec3(0, 0, 0), vec3(165, 165, 165), white), -18), vec3(130, 0, 65));
    list[i++] = new translate(new rotate_y(new box(vec3(0, 0, 0), vec3(165, 330, 165), white), 15), vec3(265, 0, 295));

    scene_object *objects = new object_list(list, n, shutter_t0, shutter_t1);

    return scene{ objects, cam };
}

scene cornell_smoke() {

    // setup camera
    vec3 cam_pos = { 278, 278, -800 };
    vec3 lookat = { 278, 278, 100 };
    vec3 up = { 0, 1, 0 };
    float vfov = 40.0f;
    float aspect = G_bufferWidth / (float) G_bufferHeight;
    float aperture = 0.00f;
    float focus_dist = (cam_pos - lookat).length();
    float shutter_t0 = 0.0f;
    float shutter_t1 = 1.0f;

    camera *cam = new camera(cam_pos, lookat, up, vfov, aspect, aperture, focus_dist, shutter_t0, shutter_t1);

    // setup scene objects
    int n = 8;
    scene_object **list = new scene_object*[n];
    int i = 0;

    material *red = new lambertian(new uni_color_tex(vec3(0.65f, 0.05f, 0.05f)));
    material *white = new lambertian(new uni_color_tex(vec3(0.73f, 0.73f, 0.73f)));
    material *green = new lambertian(new uni_color_tex(vec3(0.12f, 0.45f, 0.15f)));
    material *light = new diffuse_light(new uni_color_tex(vec3(7.0f, 7.0f, 7.0f)));

    list[i++] = new yz_rect(555, 0, 0, 555, 555, green);
    list[i++] = new yz_rect(0, 555, 0, 555, 0, red);
    //list[i++] = new xz_rect(343, 213, 227, 332, 554, light); // smaller light, needs A LOT more samples without bias
    list[i++] = new xz_rect(443, 113, 127, 432, 554, light);
    list[i++] = new xz_rect(555, 0, 0, 555, 555, white);
    list[i++] = new xz_rect(0, 555, 0, 555, 0, white);
    list[i++] = new xy_rect(555, 0, 0, 555, 555, white);
    scene_object *smoke_box1 = new translate(new rotate_y(new box(vec3(0, 0, 0), vec3(165, 165, 165), white), -18), vec3(130, 0, 65));
    scene_object *smoke_box2 = new translate(new rotate_y(new box(vec3(0, 0, 0), vec3(165, 330, 165), white), 15), vec3(265, 0, 295));
    list[i++] = new constant_volume(smoke_box1, 0.01f, new uni_color_tex(vec3(1.0f, 1.0f, 1.0f)));
    list[i++] = new constant_volume(smoke_box2, 0.01f, new uni_color_tex(vec3(0.0f, 0.0f, 0.0f)));

    scene_object *objects = new object_list(list, n, shutter_t0, shutter_t1);

    return scene{ objects, cam };
}

scene book2_final() {

    // setup camera
    vec3 cam_pos = { 450, 278, -560 };
    vec3 lookat = { 200, 278, 300 };
    vec3 up = { 0, 1, 0 };
    float vfov = 40.0f;
    float aspect = G_bufferWidth / (float) G_bufferHeight;
    float aperture = 0.00f;
    float focus_dist = (cam_pos - lookat).length();
    float shutter_t0 = 0.0f;
    float shutter_t1 = 1.0f;

    camera *cam = new camera(cam_pos, lookat, up, vfov, aspect, aperture, focus_dist, shutter_t0, shutter_t1);

    // setup scene objects
    int nb = 20;
    int ns = 1000;
    scene_object **list = new scene_object*[30];
    scene_object **boxlist = new scene_object*[nb*nb];
    scene_object **boxlist2 = new scene_object*[ns];

    int width, height, channels;
    uint8 *pixels = stbi_load("../earthmap.jpg", &width, &height, &channels, 3);
    if (!pixels) DebugBreak();
    
    material *earth = new lambertian(new image_tex(pixels, width, height));
    material *white = new lambertian(new uni_color_tex(vec3(0.73f, 0.73f, 0.73f)));
    material *green = new lambertian(new uni_color_tex(vec3(0.48f, 0.83f, 0.53f)));
    material *light = new diffuse_light(new uni_color_tex(vec3(7, 7, 7)));
    material *orange = new lambertian(new uni_color_tex(vec3(0.7f, 0.3f, 0.1f)));
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
            boxlist[b++] = new box(vec3(x0, y0, z0), vec3(x1, y1, z1), green);
        }
    }

    int l = 0;
    list[l++] = new bvh_node(boxlist, b, shutter_t0, shutter_t1); // green boxes
    list[l++] = new xz_rect(123, 423, 147, 412, 554, light); // light
    vec3 center(400, 400, 200);
    list[l++] = new sphere(center, 50, orange, center + vec3(30, 0, 0), 0, 1);            // orange-brownish sphere
    list[l++] = new sphere(vec3(260, 150, 45), 50, new dielectric(1.5f));                 // glass sphere
    list[l++] = new sphere(vec3(0, 150, 145), 50, new metal(vec3(0.8f, 0.8f, 0.9f), 0.1f));  // silver sphere
    list[l++] = new sphere(vec3(400, 200, 400), 100, earth);                              // earth sphere
    list[l++] = new sphere(vec3(220, 280, 300), 80, perlin);                              // perlin sphere


    scene_object *volume_boundary = new sphere(vec3(360, 150, 145), 70, new dielectric(1.5f));
    list[l++] = volume_boundary;
    list[l++] = new constant_volume(volume_boundary, 0.2f, new uni_color_tex(vec3(0.2f, 0.4f, 0.9f))); // blue sphere
    
    volume_boundary = new sphere(vec3(0, 0, 0), 5000, new dielectric(1.5));
    list[l++] = new constant_volume(volume_boundary, 0.0001f, new uni_color_tex(vec3(1.0f, 1.0f, 1.0f))); // fog

    // box of white spheres
    for (int i = 0; i < ns; i++) {
        boxlist2[i] = new sphere(vec3(165 * randf(), 165 * randf(), 165 * randf()), 10, white);
    }
    list[l++] = new translate(new rotate_y(new bvh_node(boxlist2, ns, shutter_t0, shutter_t1), 15), vec3(-100, 270, 395));

    scene_object *objects = new object_list(list, l, shutter_t0, shutter_t1);

    return scene{ objects, cam };
}

scene triangles() {

    // setup camera
    vec3 cam_pos = { 278, 278, -800 };
    vec3 lookat = { 278, 278, 100 };
    vec3 up = { 0, 1, 0 };
    float vfov = 40.0f;
    float aspect = G_bufferWidth / (float) G_bufferHeight;
    float aperture = 20.0f;
    float focus_dist = (cam_pos - lookat).length();
    float shutter_t0 = 0.0f;
    float shutter_t1 = 1.0f;

    camera *cam = new camera(cam_pos, lookat, up, vfov, aspect, aperture, focus_dist, shutter_t0, shutter_t1);

    // setup scene objects
    int n = 10;
    scene_object **list = new scene_object*[n];
    int i = 0;

    material *red = new lambertian(new uni_color_tex(vec3(0.65f, 0.05f, 0.05f)));
    material *white = new lambertian(new uni_color_tex(vec3(0.73f, 0.73f, 0.73f)));
    material *green = new lambertian(new uni_color_tex(vec3(0.12f, 0.45f, 0.15f)));
    material *light = new diffuse_light(new uni_color_tex(vec3(4.0f, 4.0f, 4.0f)));
    material *silver = new metal(vec3(0.8f, 0.8f, 0.9f), 0.9f);
    material *dia = new dielectric(2.4f);

    list[i++] = new yz_rect(555, 0, 0, 555, 555, green);
    list[i++] = new yz_rect(0, 555, 0, 555, 0, red);
    //list[i++] = new xz_rect(343, 213, 227, 332, 554, light); // smaller light, needs A LOT more samples without bias
    list[i++] = new xz_rect(443, 113, 127, 432, 554, light);
    list[i++] = new xz_rect(555, 0, 0, 555, 555, white);
    list[i++] = new xz_rect(0, 555, 0, 555, 0, white);
    list[i++] = new xy_rect(555, 0, 0, 555, 555, silver);
   // list[i++] = new translate(new triangle(2 * vec3(0, 0, 82.5f), 2 * vec3(82.5f, 82.5f, 120), 2 * vec3(185, 0, 0), silver), vec3(90, 0, 165));
   // list[i++] = new translate(new triangle(2 * vec3(0, 0, 0), 2 * vec3(82.5f, 82.5f, 100), 2 * vec3(165, 0, 82.5f), silver), vec3(185, 0, 95));

    size_t tris = 0;
    scene_object **bunny = readObj("..\\obj\\bunny.obj", dia, &tris, true, 2000.0f, vec3(195, -20, 280));
    if (tris && bunny) {
        list[i++] = new bvh_node(bunny, (int)tris, shutter_t0, shutter_t1);
    }
    
    tris = 0;
    scene_object **teapot = readObj("..\\obj\\teapot3_no_vt.obj", dia, &tris, false, 250.0f, vec3(285, 50, 290));
    if (tris && teapot) {
        list[i++] = new rotate_y(new bvh_node(teapot, (int) tris, shutter_t0, shutter_t1), 30);
    }

   /* tris = 0;
    scene_object **diamond = readObj("..\\obj\\diamond_pruned.obj", green, &tris, true , 1.0f, vec3(280, 70, 100));
    if (tris && diamond) {
        list[i++] = new bvh_node(diamond, (int) tris, shutter_t0, shutter_t1);
    }*/

 /*   tris = 0;
    scene_object **spider = readObj("..\\obj\\spider_pruned.obj", dia, &tris, false, 1.3f, vec3(385, 70, 100));
    if (tris && spider) {
        list[i++] = new bvh_node(spider, (int) tris, shutter_t0, shutter_t1);
    }
    */
    scene_object *objects = new object_list(list, i, shutter_t0, shutter_t1);
    
    return scene{ objects, cam };
}
