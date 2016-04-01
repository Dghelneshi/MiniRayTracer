#include <Windows.h>
#include <process.h>
#include <stdio.h>
#include <io.h> // console I/O, _open_osfhandle
#include <iostream>
#include <fcntl.h>
#include <float.h>

#include "common.h"
#include "pcg.h"
#include "material.h"
#include "scene_object.h"
#include "sphere.h"
#include "camera.h"
#include "work_queue.h"

static int32 G_windowWidth = 600;
static int32 G_windowHeight = 400;
static int32 G_bufferWidth = G_windowWidth;
static int32 G_bufferHeight = G_windowHeight;
static int32 G_samplesPerPixel = 16; // TODO: will be reduced to the next smallest square number until I implement a more robust sample distribution
static int32 G_maxBounces = 16;
static int32 G_numThreads = 0; // 0 == automatic
static int32 G_packetSize = 64;
static int32 G_updateFreq = 60;
static bool G_isRunning = true;
static volatile LONG G_numTracesDone = 0;
static uint32* G_backBuffer;

////////////////////////////
//       RAY TRACER       //
////////////////////////////

vec3 trace(const ray& r, const scene_object& scene, pcg32_random_t *rng, int32 depth) {

    hit_record rec;
    if (scene.hit(r, 0.001f, FLT_MAX, &rec)) {
        ray scattered;
        vec3 attenuation;
        if ((depth < G_maxBounces) && rec.mat_ptr->scatter(r, rec, &attenuation, rng, &scattered)) {
            return attenuation * trace(scattered, scene, rng, depth + 1);
        }
        else {
            return vec3(0, 0, 0);
        }
    }

    else { // background (sky)
        float t = 0.5f * (r.dir.y + 1.0f);
        return (1.0f - t) * vec3(1, 1, 1) + t * vec3(0.5f, 0.7f, 1.0f);
    }
}

// TODO: class, or just put it somewhere else?
struct vec2 {
    float x;
    float y;
};

// worker thread arguments
struct drawArgs {
    uint64 initstate;
    uint64 initseq;
    work_queue* queue;
    scene_object *scene;
    camera *camera;
    vec2 *sample_dist; // array of sample offsets
    int32 numSamples;
};

// main worker thread function
unsigned int __stdcall draw(void * argp) {

    drawArgs args = *(drawArgs*) argp;
    
    pcg32_random_t rng = {};
    pcg32_srandom_r(&rng, args.initstate, args.initseq);

    while (work *work = args.queue->getWork()) // fetch new work from the queue
    {
        for (int32 y = work->yMin; y < work->yMax; y++) {

            for (int32 x = work->xMin; x < work->xMax; x++) {

                vec3 color(0, 0, 0);

                // multiple samples per pixel
                for (int i = 0; i < args.numSamples; i++)
                {
                    float u = (x + args.sample_dist[i].x) / (float) G_bufferWidth;
                    float v = (y + args.sample_dist[i].y) / (float) G_bufferHeight;

                    ray r = args.camera->get_ray(u, v, &rng);

                    color += trace(r, *args.scene, &rng, 0);

                }
                color /= float(args.numSamples);

                color.gamma_correct();

                uint32 red   = (uint32) (255.99f * color.r);
                uint32 green = (uint32) (255.99f * color.g);
                uint32 blue  = (uint32) (255.99f * color.b);

                G_backBuffer[x + y * G_bufferWidth] = (red << 16) | (green << 8) | blue;
            }

            // periodically check if we want to exit prematurely
            if (!G_isRunning) {
                _endthreadex(0);
            }
        }
    }

    InterlockedIncrement(&G_numTracesDone);
    return 0;
}


/////////////////////////
// COMMAND LINE PARSER //
/////////////////////////

#define MAX_NUM_ARGVS 64

static int G_argc = 1;
static char *G_argv[MAX_NUM_ARGVS] = { 0 };

int CheckParm(const char *parm) {

    for (int i = 1; i < G_argc; i++) {
        if (strcmp(parm, G_argv[i]) == 0) {
            return i;
        }
    }
    return 0;
}

void ParseCmdLine(char* lpCmdLine) {
    while (*lpCmdLine && (G_argc < MAX_NUM_ARGVS)) {

        while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126))) {
            lpCmdLine++;
        }

        if (*lpCmdLine) {
            G_argv[G_argc] = lpCmdLine;
            G_argc++;

            while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126))) {
                lpCmdLine++;
            }

            if (*lpCmdLine) {
                *lpCmdLine = 0;
                lpCmdLine++;
            }
        }
    }
}

bool ApplyInt32Parameter(const char* string, int32 *target, int32 min = 1, int32 max = INT32_MAX) {
    int index = CheckParm(string);
    if ((index > 0) && (G_argc > (index + 1))) {
        int32 value = atoi(G_argv[index + 1]);
        if ((value >= min) && (value <= max)) {
            *target = value;
            return true;
        }
    }
    return false;
}

void ApplyCmdLine() {

    if (ApplyInt32Parameter("-width",  &G_windowWidth))
        G_bufferWidth = G_windowWidth;
    if (ApplyInt32Parameter("-height", &G_windowHeight))
        G_bufferHeight = G_windowHeight;
    ApplyInt32Parameter("-samples",    &G_samplesPerPixel);
    ApplyInt32Parameter("-packetsize", &G_packetSize);
    ApplyInt32Parameter("-threads",    &G_numThreads, 0);
    ApplyInt32Parameter("-depth",      &G_maxBounces);

    int printHelp = CheckParm("-help") || CheckParm("-?");
    if (printHelp > 0) {
        printf_s("\nAvailable Parameters:\n-width -height -samples -packetsize -threads -depth\n");
        exit(0);
    }
}


////////////////////////////
//          MAIN          //
////////////////////////////

LRESULT CALLBACK MainWndProc(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    LRESULT result = 0;

    switch (uMsg) {
    case WM_DESTROY:
    case WM_CLOSE: {
        G_isRunning = false; // will exit the program soon(tm)!
    } break;

    default:
        result = DefWindowProc(hWindow, uMsg, wParam, lParam);
    }

    return result;
}

scene_object *random_scene(int n, float camera_t0, float camera_t1, pcg32_random_t *rng) {

    scene_object **list = new scene_object*[n + 6];
    
    list[0] = new sphere(vec3(0, -1000, 0), 1000, new lambertian(vec3(0.5, 0.5, 0.5)));

    int half_sqrt_n = int(sqrtf(float(n)) * 0.5f);
    int i = 1;
    for (int a = -half_sqrt_n; a < half_sqrt_n; a++) {

        for (int b = -half_sqrt_n; b < half_sqrt_n; b++) {

            float choose_mat = randf(rng);
            vec3 center(a + 0.9f * randf(rng), 0.2f, b + 0.9f * randf(rng));

            if ((center - vec3(4, 0.2f, 0)).length() > 0.9f) {

                material *mat;
                sphere *sphere;

                if (choose_mat < 0.5f) {
                    mat = new lambertian(vec3(randf(rng)*randf(rng), randf(rng)*randf(rng), randf(rng)*randf(rng)));
                    sphere = new class sphere(center, 0.2f, mat, center + vec3{ 0, 0.5f*randf(rng), 0 }, 0.0f, 1.0f);
                }
                else if (choose_mat < 0.9f) {
                    mat = new metal(0.5f * vec3(1 + randf(rng), 1 + randf(rng), 1 + randf(rng)), randf(rng));
                    sphere = new class sphere(center, 0.2f, mat);
                }
                else {
                    mat = new dielectric(1.4f + randf(rng));
                    sphere = new class sphere(center, 0.2f, mat);
                }

                list[i++] = sphere;
            }
        }
    }

    list[i++] = new sphere(vec3(0, 1, 0),   1.0f, new dielectric(1.5f));
    list[i++] = new sphere(vec3(-4, 1, 0),  1.0f, new lambertian(vec3(0.4f, 0.2f, 0.1f)));
    list[i++] = new sphere(vec3(4, 1, 0),   1.0f, new metal(vec3(0.7f, 0.6f, 0.5f), 1.0f));
    list[i++] = new sphere(vec3(4, 1, 3),   1.0f, new dielectric(2.4f));
    list[i++] = new sphere(vec3(4, 1, 3), -0.95f, new dielectric(2.4f));

    // 600x400x16 clang++
    // n:        500 |  1000 | 10000 | 100000
    // list:   16.59 | 32.23 | too damn long
    // bvh:     2.54 |  2.83 |  3.87 |   6.56 (+0.35 precalc)
    // bvh re:  

    //return new object_list(list, i, camera_t0, camera_t1);
    return new bvh_node(list, i, camera_t0, camera_t1, rng);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    // get parent console to print to stdout if needed
    BOOL bConsole = AttachConsole(ATTACH_PARENT_PROCESS);
    if (bConsole)
    {
        HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
        int fd = _open_osfhandle((intptr_t) hStdOut, _O_TEXT);
        if (fd > 0)
        {
            *stdout = *_fdopen(fd, "w");
            setvbuf(stdout, NULL, _IONBF, 0);
        }
        std::ios::sync_with_stdio();
    }

    ParseCmdLine(lpCmdLine);
    ApplyCmdLine();

    if (bConsole) FreeConsole();

    WNDCLASSEX windowClass = { sizeof(windowClass) };
    windowClass.lpfnWndProc = MainWndProc;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = "MiniRayTracerMain";

    if (!RegisterClassEx(&windowClass)) {
        DebugBreak();
        exit(EXIT_FAILURE);
    }

    DWORD windowStyle = WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;
    DWORD windowStyleEx = 0;

    // adjust window so that drawable area is exactly the size we want
    RECT r = { 0, 0, G_windowWidth, G_windowHeight };
    AdjustWindowRectEx(&r, windowStyle, FALSE, windowStyleEx);

    if ((r.right - r.left) == G_windowWidth) { // AdjustWindowRect doesn't seem to work for some window styles
        int32 x_adjust = GetSystemMetrics(SM_CXFIXEDFRAME) * 2;
        int32 y_adjust = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFIXEDFRAME) * 2;
        r.right += x_adjust;
        r.bottom += y_adjust;
    }

    HWND mainWindow = CreateWindowEx(windowStyleEx, windowClass.lpszClassName, "MiniRayTracer", windowStyle, 0, 0,
                                     (r.right - r.left), (r.bottom - r.top), NULL, NULL, hInstance, 0);

    // tells Windows how to interpret our backbuffer
    BITMAPINFO bmpInfo;
    bmpInfo.bmiHeader.biSize = sizeof(bmpInfo.bmiHeader);
    bmpInfo.bmiHeader.biWidth = G_bufferWidth;
    bmpInfo.bmiHeader.biHeight = G_bufferHeight; // y axis up
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biBitCount = 32;
    bmpInfo.bmiHeader.biCompression = BI_RGB;

    G_backBuffer = (uint32*) calloc(G_bufferWidth * G_bufferHeight, 4);

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

    camera *camera = new class camera(cam_pos, lookat, up, vfov, aspect, aperture, focus_dist, shutter_t0, shutter_t1);

    // setup scene
    SetWindowTextA(mainWindow, "Generating Scene...");
    
    // start timer for scene generation
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    LARGE_INTEGER t1_gen;
    QueryPerformanceCounter(&t1_gen);

    pcg32_random_t rng = {};
    pcg32_srandom_r(&rng, 0xabcd571f2851b2a5ULL, 0xabc0fe8761cbafe9ULL);

    scene_object *scene = random_scene(500, shutter_t0, shutter_t1, &rng);
    
    // stop timer, display in window title
    LARGE_INTEGER t2_gen;
    QueryPerformanceCounter(&t2_gen);

    char windowTitle[64];
    sprintf_s(windowTitle, 64, "MiniRayTracer - Scene Gen: %.4fs", float(t2_gen.QuadPart - t1_gen.QuadPart) / freq.QuadPart);
    SetWindowTextA(mainWindow, windowTitle);

    // setup sample distribution
    int32 sqrt_samples = (int32) sqrt((float) G_samplesPerPixel); // TODO: distribution for non-square numbers
    int32 numSamples = sqrt_samples * sqrt_samples;

    vec2 *sample_dist = (vec2*) calloc(numSamples, sizeof(*sample_dist));

    for (int i = 0; i < sqrt_samples; i++)
    {
        for (int j = 0; j < sqrt_samples; j++)
        {
            // sample distribution is a regular grid
            float u_adjust = (i + 0.5f) / (float) sqrt_samples;
            float v_adjust = (j + 0.5f) / (float) sqrt_samples;
            sample_dist[i * sqrt_samples + j].x = u_adjust;
            sample_dist[i * sqrt_samples + j].y = v_adjust;
        }
    }
    
    // multi-threading stuff

    if (G_numThreads == 0) {
        // ALL YOUR PROCESSOR ARE BELONG TO US!
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        G_numThreads = sysInfo.dwNumberOfProcessors;
        if (G_numThreads < 1) // just in case...
            G_numThreads = 1;
    }

    // threads can get small packets of work from here
    work_queue *queue = new work_queue(G_bufferWidth, G_bufferHeight, G_packetSize);

    // setup function arguments for the worker threads
    drawArgs *threadArgs = (drawArgs*) calloc(G_numThreads, sizeof(*threadArgs));
    for (int i = 0; i < G_numThreads; i++)
    {
        // thread arguments are all the same for now
        threadArgs[i].initstate = 0x1234567890abcdefULL;
        threadArgs[i].initseq = 0xfedcba0987654321ULL;
        threadArgs[i].queue = queue;
        threadArgs[i].camera = camera;
        threadArgs[i].scene = scene;
        threadArgs[i].sample_dist = sample_dist;
        threadArgs[i].numSamples = numSamples;
    }

    InterlockedAnd(&G_numTracesDone, 0);

    // start time for ray tracer
    LARGE_INTEGER t1_trace;
    QueryPerformanceCounter(&t1_trace);

    // start worker threads
    HANDLE *threads = (HANDLE*) calloc(G_numThreads, sizeof(*threads));
    for (int i = 0; i < G_numThreads; i++)
    {
        threads[i] = (HANDLE) _beginthreadex(NULL, 0, &draw, &threadArgs[i], 0, NULL);
    }

    // main loop, draws current picture in window
    while (G_isRunning) {

        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        Sleep(1000 / G_updateFreq);
        
        if (G_numTracesDone < G_numThreads) {
        
            // display elapsed time in window title
            LARGE_INTEGER t2_trace;
            QueryPerformanceCounter(&t2_trace);

            char frameInfo[128];
            sprintf_s(frameInfo, 128, "%s - Trace: %.2fs", windowTitle, float(t2_trace.QuadPart - t1_trace.QuadPart) / freq.QuadPart);
            SetWindowTextA(mainWindow, frameInfo);
        }
        else {
            G_updateFreq = 30;
        }

        // draw current backbuffer to window
        HDC DC = GetDC(mainWindow);
        StretchDIBits(DC, 0, 0, G_windowWidth, G_windowHeight, 0, 0, G_bufferWidth, G_bufferHeight, G_backBuffer, &bmpInfo, DIB_RGB_COLORS, SRCCOPY);
        ReleaseDC(mainWindow, DC);
    }

    // wait for threads to finish, but terminate forcefully if too slow
    DWORD waitResult = WaitForMultipleObjects(G_numThreads, threads, TRUE, 500);
    if (waitResult == WAIT_TIMEOUT) {
        for (int32 i = 0; i < G_numThreads; i++)
        {
            TerminateThread(threads[i], 1);
        }
    }
    for (int32 i = 0; i < G_numThreads; i++)
    {
        CloseHandle(threads[i]);
    }

    return 0;
}