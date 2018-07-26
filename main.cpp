#include <limits>
#include <thread>
#include <stdio.h>

#include "common.h"

#include "platform.h"
#include "main.h"

#include "pcg.h"
#include "all_scene_objects.h"
#include "camera.h"
#include "obj_loader.h"
#include "work_queue.h"
#include "pdf.h"
#include "scene.h"
#include "cmdline_parser.h"

using namespace MRT;

/* TODO:
    - most scenes are currently broken due to handling of sky + tone mapping!
    - proper SIMD implementation via ray bundles
        - GPU implementation?
    - fix build with MinGW headers, fix build with GCC
    - could eliminate arbitrary ray tmin offset by using the isInside property to only intersect with front XOR backfaces
        - objects inside other objects could be supported by remembering the last intersected object
    - add support for spectral rendering
    - create menu to select scenes, change parameters, etc., think about more effects that can be done in post
    - construct test scene in pbrt as ground truth
    - HDR / tone mapping: Make const parameter configurable per-scene and at runtime!
    - check everywhere whether hitting backfaces from inside solid volumes is handled correctly
    - current Vec4/Vec3 setup can lead to very subtle bugs (e.g. Vec3 + float -> Vec4 add -> w != 0)
    - add benchmark for iterations over the buffer (standard x,y loop, single counter, pointer, pointer in reverse)
    - fix race condition in work queue
    - combine draw and draw2 into a common interface
    - complete math library
    - try to make a very simple brute force SSS material
    - do something to combat the "fireflies"
    - allocate obj file triangles and/or BVH nodes into contiguous memory --> SoA index buffers in a Mesh class
    - press key to pause/continue tracing (even after initial image is done)
    - iterative trace function?
    - generalize moving object code (move into base class, add transforms for all objects, can also use this for instancing)
    - consistent naming conventions everywhere
    - better/any documentation
    - clean up function names, eliminate globals
    - more error checking & user feedback (e.g. obj file not found)
    - Ctrl+Shift+F: TODO
*/

static volatile bool G_isRunning = true;
static std::atomic<size_t> G_rayCounter;

static uint32* G_backBuffer; // ARGB in register, BGRA in memory
static Vec3 *G_linearBackBuffer;

////////////////////////////
//       RAY TRACER       //
////////////////////////////

Vec3 trace(const ray& r, const scene_object& scene, scene_object *biased_obj, uint32 depth) {

    G_rayCounter.fetch_add(1, std::memory_order_relaxed);
    static MRT_Params *p = getParams();

    hit_record hrec;
    if (scene.hit(r, 0.001f, std::numeric_limits<float>::max(), &hrec)) {
        
        thread_local pdf_space pdf_storage;
        thread_local pdf * const pdf_p = (pdf*) &pdf_storage;
        scatter_record srec;

        Vec3 emitted = hrec.mat_ptr->sampleEmissive(r, hrec);

        if ((depth < p->maxBounces) && hrec.mat_ptr->scatter(r, hrec, &srec, pdf_p)) {

            if (srec.is_specular) {
                return srec.attenuation * trace(srec.specular_ray, scene, biased_obj, depth + 1);
            }
            else {
                ray scattered;
                float pdf_v;
                if (biased_obj) {
                    object_pdf plight(hrec.p, biased_obj);
                    mix_pdf p(&plight, pdf_p);
                    scattered = ray(hrec.p, p.generate(r.time), r.time);
                    pdf_v = p.value(scattered.dir, r.time);
                }
                else {
                    scattered = ray(hrec.p, pdf_p->generate(r.time), r.time);
                    pdf_v = pdf_p->value(scattered.dir, r.time);
                }
                //delete srec.pdf; // NOTE: currently reusing thread local storage as we don't need more than one PDF per thread at a time

                float scatter_pdf = hrec.mat_ptr->scattering_pdf(r, hrec, scattered);
                Vec3 scatter_color = trace(scattered, scene, biased_obj, depth + 1);

                return emitted + srec.attenuation * scatter_pdf * scatter_color / pdf_v;
            }
        }
        else {
            return emitted;
        }
    }
    else {
        if (p->sceneSelect >= SCENE_CORNELL_BOX)
            return Vec3(0.0f);
        else {
            // background (sky)
            float t = 0.5f * (r.dir.y + 1.0f);
            return Vec3(1.0f - t) + t * Vec3(0.5f, 0.7f, 1.0f);
        }
    }
}

// TODO: delete once we have sobol sequence
struct vec2 {
    float x;
    float y;
};

// worker thread arguments
struct drawArgs {
    uint64 initstate;
    uint64 initseq;
    work_queue* queue;
    scene scene;
    vec2 *sample_dist; // array of sample offsets
    uint32 numSamples;
    uint32 threadId;
};

// main worker thread function
unsigned int __stdcall draw(void * argp) {

    MRT_LowerThreadPriority(); // ensure main thread gets enough CPU time

    drawArgs args = *(drawArgs*) argp;
    Init_Thread_RNG(args.initstate, args.initseq);
    MRT_Params *p = getParams();

    while (tile *t = args.queue->getWork(nullptr)) // fetch new work from the queue
    {
        for (uint32 y = t->yMin; y < t->yMax; y++) {
            for (uint32 x = t->xMin; x < t->xMax; x++) {

                Vec3 color(0, 0, 0);

                // multiple samples per pixel
                for (uint32 i = 0; i < args.numSamples; i++)
                {
                    float u = (x + args.sample_dist[i].x) / (float) p->bufferWidth;
                    float v = (y + args.sample_dist[i].y) / (float) p->bufferHeight;

                    ray r = args.scene.camera->get_ray(u, v);

                    Vec3 sample = trace(r, *args.scene.objects, args.scene.biased_objects, 0);

                    if (!std::isfinite(sample.r) || !std::isfinite(sample.g) || !std::isfinite(sample.b)) {
                        sample = color;
                    }
                    color += sample;
                }
                color /= float(args.numSamples);

                float lum = luminance(color);
                if (lum > p->maxLuminance) {
                    color = color * (p->maxLuminance / lum);
                }

                G_linearBackBuffer[x + y * p->bufferWidth] = color;
                //G_backBuffer[x + y * p->bufferWidth] = ARGB32(color.gamma_correct());
            }

            // periodically check if we want to exit prematurely
            if (!G_isRunning) {
                goto endthread;
            }
        }
    }

endthread:
    return 0;
}

// --- different multi-threading implementation ---

// main worker thread function
unsigned int __stdcall draw2(void * argp) {

    MRT_LowerThreadPriority(); // ensure main thread gets enough CPU time

    drawArgs args = *(drawArgs*) argp;
    Init_Thread_RNG(args.initstate, args.initseq);
    MRT_Params *p = getParams();

    uint32 sampleCount = 0;
    while (tile *t = args.queue->getWork(&sampleCount)) // fetch new work from the queue
    {
        for (uint32 y = t->yMin; y < t->yMax; y++) {
            for (uint32 x = t->xMin; x < t->xMax; x++) {

                float u = (x + args.sample_dist[sampleCount].x) / (float) p->bufferWidth;
                float v = (y + args.sample_dist[sampleCount].y) / (float) p->bufferHeight;

                ray r = args.scene.camera->get_ray(u, v);

                Vec3 color = trace(r, *args.scene.objects, args.scene.biased_objects, 0);

                if (!std::isfinite(color.r) || !std::isfinite(color.g) || !std::isfinite(color.b)) {
                    if (sampleCount > 0)
                        color = G_linearBackBuffer[x + y * p->bufferWidth];
                    else
                        color = Vec3(0.0f);
                }

                if (sampleCount > 0) {
                    Vec3 old_color = G_linearBackBuffer[x + y * p->bufferWidth];
                    color = old_color + (color - old_color) * (1.0f / (sampleCount + 1.0f)); // iterative average
                }

                float lum = luminance(color);
                if (lum > p->maxLuminance) {
                    color = color * (p->maxLuminance / lum);
                }
                
                G_linearBackBuffer[x + y * p->bufferWidth] = color;
                //G_backBuffer[x + y * p->bufferWidth] = ARGB32(color.gamma_correct());
            }
            // periodically check if we want to exit prematurely
            if (!G_isRunning) {
                goto endthread;
            }
        }
    }

endthread:
    return 0;
}


////////////////////////////
//          INPUT         //
////////////////////////////

static KeyState lButtonState = MRT_NONE;
static KeyState rButtonState = MRT_NONE;
//static KeyState ctrlState    = MRT_NONE;
//static KeyState shiftState   = MRT_NONE;
//static KeyState altState     = MRT_NONE;

void MRT::MouseCallback(int32 x, int32 y, KeyState lButton, KeyState rButton) {
    if (lButton != MRT_NONE) lButtonState = lButton;
    if (rButton != MRT_NONE) rButtonState = rButton;
}

void MRT::KeyboardCallback(int keycode, KeyState state, KeyState prev) {
    static MRT_Params *p = getParams();
    if (state == MRT_DOWN && prev == MRT_UP)
        p->delay = false;
    switch (keycode) {
    case 'a':
        break; // TODO
    }
}

void MRT::WindowCallback(WindowEvent e) {
    switch (e) {
    case MRT_CLOSE:
        G_isRunning = false;
        break;
    }
}

////////////////////////////
//          MAIN          //
////////////////////////////

int main(int argc, char* argv[]) {
    
    MRT_PlatformInit();

    ParseArgv(argc, argv);

    MRT_Params *p = getParams();

    MRT_CreateWindow(p->windowWidth, p->windowHeight, p->bufferWidth, p->bufferHeight);

    G_backBuffer = (uint32*) calloc(p->bufferWidth * p->bufferHeight, sizeof(*G_backBuffer));
    MRT_DrawToWindow(G_backBuffer);

    G_linearBackBuffer = (Vec3*) calloc(p->bufferWidth * p->bufferHeight, sizeof(*G_linearBackBuffer));

    /////////////////////////
    // --- Setup Scene --- //
    /////////////////////////

    Init_Thread_RNG(11350390909718046443uLL, 6305599193148252115uLL);

    MRT_SetWindowTitle("MiniRayTracer - Generating Scene...");

    // start timer for scene generation
    uint64 t1_gen = MRT_GetTime();

    scene scene = select_scene((scenes) p->sceneSelect, float(p->bufferWidth) / float(p->bufferHeight));

    // stop timer, display in window title
    char windowTitle[64];
    snprintf(windowTitle, sizeof(windowTitle), "MiniRayTracer - Scene: %.0fms", 1000.f * MRT_TimeDelta(t1_gen, MRT_GetTime()));
    MRT_SetWindowTitle(windowTitle);

    // setup sample distribution
    // TODO: distribution for non-square numbers
    //       unbiased distribution that converges earlier: Sobol sequence or others, see http://woo4.me/wootracer/2d-samplers/
    uint32 sqrt_samples = (uint32) MRT::sqrt((float) p->samplesPerPixel);
    uint32 numSamples = sqrt_samples * sqrt_samples;

    vec2 *sample_dist = (vec2*) calloc(numSamples, sizeof(*sample_dist));

    for (uint32 i = 0; i < sqrt_samples; i++) {
        for (uint32 j = 0; j < sqrt_samples; j++) {
            // sample distribution is a regular grid
            float u_adjust = (i + 0.5f) / (float) sqrt_samples;
            float v_adjust = (j + 0.5f) / (float) sqrt_samples;
            sample_dist[i * sqrt_samples + j].x = u_adjust;
            sample_dist[i * sqrt_samples + j].y = v_adjust;
        }
    }

    /////////////////////////////
    // --- Multi-Threading --- //
    /////////////////////////////

    if (p->numThreads == 0) {
        // ALL YOUR PROCESSOR ARE BELONG TO US!
        p->numThreads = std::thread::hardware_concurrency();
    }
    
    typedef unsigned int(__stdcall *thread_fn)(void*);
    thread_fn thread_fun;
    work_queue *queue;

    if (p->threadingMode == 0) {
        thread_fun = draw;
        queue = new work_queue_seq(p->bufferWidth, p->bufferHeight, p->tileSize, p->numThreads);
    }
    else if (p->threadingMode == 1) {
        thread_fun = draw2;
        queue = new work_queue_dynamic(p->bufferWidth, p->bufferHeight, p->tileSize, p->numThreads, numSamples);
    }

    // setup function arguments for the worker threads
    drawArgs *threadArgs = (drawArgs*) calloc(p->numThreads, sizeof(drawArgs));
    for (uint32 i = 0; i < p->numThreads; i++) {
        threadArgs[i].initstate = (uint64(rand32()) << 32) | rand32();
        threadArgs[i].initseq   = (uint64(rand32()) << 32) | rand32();
        threadArgs[i].queue = queue;
        threadArgs[i].scene = scene;
        threadArgs[i].sample_dist = sample_dist;
        threadArgs[i].numSamples = numSamples;
        threadArgs[i].threadId = i;
    }

    // delayed start for recording
    while (p->delay && G_isRunning) {
        MRT_HandleMessages();
        MRT_Sleep(33);
    }

    // start time for ray tracer
    uint64 t1_trace = MRT_GetTime();

    // start worker threads
    std::thread *threads = new std::thread[p->numThreads];
    for (size_t i = 0; i < p->numThreads; i++) {
        void* args = &threadArgs[i];
        threads[i] = std::thread(thread_fun, args);
    }

    static uint32 updateFreq = 30;
    bool isTracing = true;
    
    while (G_isRunning) {

        MRT_HandleMessages();
        MRT_Sleep(1000u / updateFreq);

        if (isTracing) {
            // display elapsed time in window title
            float secondsElapsed = MRT_TimeDelta(t1_trace, MRT_GetTime());
            float pctDone = queue->getPercentDone();

            static char buf[128];

            if (pctDone == 100.0f) { // ray tracer is done!
                isTracing = false;
                updateFreq = 30;

                size_t rays = G_rayCounter;
                snprintf(buf, sizeof(buf), "%s - Trace: %.2fs - %.3f Mrays/s | %.3f us/ray\n",
                         windowTitle, secondsElapsed, ((rays * 0.000001f) / secondsElapsed), (secondsElapsed * 1000000.0f) / rays);
                MRT_SetWindowTitle(buf);
            }
            else {
                float eta = secondsElapsed * (100.0f / pctDone) - secondsElapsed;
                snprintf(buf, sizeof(buf), "%s - Trace: %.2fs (%.0f%% - ETA %.0fs)", windowTitle, secondsElapsed, pctDone, eta);
                MRT_SetWindowTitle(buf);
            }

#if 1
            {
                // Adaptive Logarithmic Mapping For Displaying Contrast Scenes
                // http://resources.mpi-inf.mpg.de/tmo/logmap/logmap.pdf

                float L_dmax = 230.0f; // reference maximum display brightness in cd/m^2
                float bias = logf(0.7f) / logf(0.5f); // tune the numerator!

                float L_wmax = 0;
                for (size_t y = 0; y < p->bufferHeight; y++) {
                    for (size_t x = 0; x < p->bufferWidth; x++) {
                        float lum = luminance(G_linearBackBuffer[x + y * p->bufferWidth]);
                        L_wmax = std::max(L_wmax, lum);
                    }
                }
                float invlogmax = 1.0f / log10f(L_wmax + 1.0f);
                float invmax = 1.0f / L_wmax;

                for (size_t y = 0; y < p->bufferHeight; y++) {
                    for (size_t x = 0; x < p->bufferWidth; x++) {
                        Vec3 color = G_linearBackBuffer[x + y * p->bufferWidth];
                        float lum = luminance(color);
                        float loglw = logf(lum + 1.0f);
                        float lum_new = (L_dmax * 0.01f * invlogmax) * (loglw / logf(2 + pow(lum * invmax, bias) * 8));
                        color = (lum_new * color) / (lum + 0.00001f);
                        G_backBuffer[x + y * p->bufferWidth] = ARGB32(color);
                    }
                }
            }
#elif 1
            {
                // Photographic Tone Reproduction for Digital Images
                // http://www.cs.utah.edu/~reinhard/cdrom/tonemap.pdf

                float a = 0.10f; // "key value" (middle gray)
                float sigma = 0.00001f;
                float scale = 1.0f / (p->bufferWidth * p->bufferHeight);
                float logavg = 0;
                float L_wmax = 0;
                for (size_t y = 0; y < p->bufferHeight; y++) {
                    for (size_t x = 0; x < p->bufferWidth; x++) {
                        float lum = luminance(G_linearBackBuffer[x + y * p->bufferWidth]);
                        logavg += logf(sigma + lum);
                        L_wmax = std::max(L_wmax, lum);
                    }
                }
                logavg = exp(scale * logavg); // NOTE: in the paper, 1/N (scale) is in the wrong place, producing inf/nan values
                float invlogavg = 1.0f / logavg;
                float invmax = 1.0f / L_wmax;

                for (size_t y = 0; y < p->bufferHeight; y++) {
                    for (size_t x = 0; x < p->bufferWidth; x++) {
                        Vec3 color = G_linearBackBuffer[x + y * p->bufferWidth];
                        float lum = luminance(color);
                        float lum_new = a * invlogavg * lum;
                        lum_new = lum_new * (1 + lum_new * (invmax*invmax)) / (1 + lum_new);
                        color = (lum_new * color) / (lum + sigma);
                        G_backBuffer[x + y * p->bufferWidth] = ARGB32(color);
                    }
                }
            }
#else
            // simple gamma correction
            for (size_t y = 0; y < p.bufferHeight; y++) {
                for (size_t x = 0; x < p->bufferWidth; x++) {
                    G_backBuffer[x + y * p->bufferWidth] = ARGB32(gamma_correct(G_linearBackBuffer[x + y * p->bufferWidth]));
                }
            }
#endif
        }

        MRT_DrawToWindow(G_backBuffer);
    }

    // wait for threads to finish
    for (size_t i = 0; i < p->numThreads; i++) {
        threads[i].join();
    }

    MRT_PlatformDestroy();

    return 0;
}