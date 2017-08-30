#define NOMINMAX
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
#include "rect.h"
#include "box.h"
#include "volumes.h"
#include "camera.h"
#include "triangle.h"
#include "obj_loader.h"
#include "work_queue.h"
#include "pdf.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


/* TODO:
    - do something to combat the "fireflies"
    - HDR / tone mapping support
    - separation of Win32 code to make porting easier
    - generalize moving object code (move into base class, add origin for all objects, maybe try pointer to struct member to save space on non-moving objects)
    - consistent naming conventions everywhere
    - better/any documentation
    - clean up function names, eliminate globals
    - separate into multiple cpp files
    - more error checking & user feedback (e.g. obj file not found)
    - Ctrl+Shift+F: TODO
*/

static uint32 G_windowWidth = 500;
static uint32 G_windowHeight = 500;
static uint32 G_bufferWidth = G_windowWidth;
static uint32 G_bufferHeight = G_windowHeight;
static uint32 G_samplesPerPixel = 32; // TODO: will be reduced to the next smallest square number until I implement a more robust sample distribution
static uint32 G_maxBounces = 32;
static uint32 G_numThreads = 1; // 0 == automatic
static uint32 G_packetSize = 32;
static uint32 G_threadingMode = 0;
static bool G_isRunning = true;
static bool G_delay = false; // delayed start for recording
static volatile uint32 *G_workDoneCounter;
static uint32* G_backBuffer;
static vec3 *G_linearBackBuffer;

#include "scenes.h"
static uint32 G_sceneSelect = SCENE_TRIANGLES;

////////////////////////////
//       RAY TRACER       //
////////////////////////////

vec3 trace(const ray& r, const scene_object& scene, scene_object *biased_obj, pcg32_random_t *rng, uint32 depth) {

    hit_record hrec;
    if (scene.hit(r, 0.001f, FLT_MAX, &hrec)) {
        scatter_record srec;
        vec3 emitted = hrec.mat_ptr->sampleEmissive(r, hrec);

        if ((depth < G_maxBounces) && hrec.mat_ptr->scatter(r, hrec, &srec, rng)) {

            if (srec.is_specular) {
                return srec.attenuation * trace(srec.specular_ray, scene, biased_obj, rng, depth + 1);
            }
            else {
                ray scattered;
                float pdf_v;
                if (biased_obj) {
                    object_pdf plight(hrec.p, biased_obj);
                    mix_pdf p(&plight, srec.pdf);
                    scattered = ray(hrec.p, p.generate(r.time, rng), r.time);
                    pdf_v = p.value(scattered.dir, r.time);
                }
                else {
                    scattered = ray(hrec.p, srec.pdf->generate(r.time, rng), r.time);
                    pdf_v = srec.pdf->value(scattered.dir, r.time);
                }
                //delete srec.pdf; // NOTE: currently using placement new into a buffer inside scatter_record

                float scatter_pdf = hrec.mat_ptr->scattering_pdf(r, hrec, scattered);
                vec3 scatter_color = trace(scattered, scene, biased_obj, rng, depth + 1);

                return emitted + srec.attenuation * scatter_pdf * scatter_color / pdf_v;
            }
        }
        else {
            return emitted;
        }
    }
    else {
        if (G_sceneSelect >= SCENE_CORNELL_BOX)
            return vec3(0.0f);
        else {
            // background (sky)
            float t = 0.5f * (r.dir.y + 1.0f);
            return (1.0f - t) * vec3(1.0f) + t * vec3(0.5f, 0.7f, 1.0f);
        }
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
    scene scene;
    vec2 *sample_dist; // array of sample offsets
    uint32 numSamples;
    uint32 threadId;
};

// main worker thread function
unsigned int __stdcall draw(void * argp) {

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL); // ensure main thread gets enough CPU time

    drawArgs args = *(drawArgs*) argp;

    pcg32_random_t rng = {};
    pcg32_srandom_r(&rng, args.initstate, args.initseq);

    while (work *work = args.queue->getWork()) // fetch new work from the queue
    {
        for (uint32 y = work->yMin; y < work->yMax; y++) {
            for (uint32 x = work->xMin; x < work->xMax; x++) {

                vec3 color(0, 0, 0);

                // multiple samples per pixel
                for (uint32 i = 0; i < args.numSamples; i++)
                {
                    float u = (x + args.sample_dist[i].x) / (float) G_bufferWidth;
                    float v = (y + args.sample_dist[i].y) / (float) G_bufferHeight;

                    ray r = args.scene.camera->get_ray(u, v, &rng);

                    color += trace(r, *args.scene.objects, args.scene.biased_objects, &rng, 0);
                }
                color /= float(args.numSamples);

                color = vmin(color, vec3(1.0f));
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
        G_workDoneCounter[args.threadId]++;
    }

    return 0;
}

// --- different multi-threading implementation ---

// worker thread arguments
struct draw2Args {
    uint64 initstate;
    uint64 initseq;
    work_queue_multi* queue;
    scene scene;
    vec2 *sample_dist; // array of sample offsets
    uint32 numSamples;
    int32 threadId;
};

// main worker thread function
unsigned int __stdcall draw2(void * argp) {

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL); // ensure main thread gets enough CPU time

    draw2Args args = *(draw2Args*) argp;

    pcg32_random_t rng = {};
    pcg32_srandom_r(&rng, args.initstate, args.initseq);
    
    uint32 sampleCount = 0;
    while (work *work = args.queue->getWork(args.threadId, &sampleCount)) // fetch new work from the queue
    {
        for (uint32 y = work->yMin; y < work->yMax; y++) {
            for (uint32 x = work->xMin; x < work->xMax; x++) {

                float u = (x + args.sample_dist[sampleCount].x) / (float) G_bufferWidth;
                float v = (y + args.sample_dist[sampleCount].y) / (float) G_bufferHeight;

                ray r = args.scene.camera->get_ray(u, v, &rng);

                vec3 color = trace(r, *args.scene.objects, args.scene.biased_objects, &rng, 0);

                if (!std::isfinite(color.r) || !std::isfinite(color.g) || !std::isfinite(color.b)) {
                    if (sampleCount > 0)
                        color = G_linearBackBuffer[x + y * G_bufferWidth];
                    else
                        color = vec3(0.0f);
                }
              
                if (sampleCount > 0) {
                    vec3 old_color = G_linearBackBuffer[x + y * G_bufferWidth];
                    color = old_color + (color - old_color) * 1.0f / (sampleCount + 1.0f); // iterative average
                }

                G_linearBackBuffer[x + y * G_bufferWidth] = color;

                color = vmin(color, vec3(1.0f));
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
        G_workDoneCounter[args.threadId]++;
    }
    if (sampleCount != (args.numSamples - 1)) DebugBreak();

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

bool ApplyUInt32Parameter(const char* string, uint32 *target, uint32 min = 1, uint32 max = UINT32_MAX) {
    int index = CheckParm(string);
    if ((index > 0) && (G_argc > (index + 1))) {
        uint32 value = atoi(G_argv[index + 1]);
        if ((value >= min) && (value <= max)) {
            *target = value;
            return true;
        }
    }
    return false;
}

void ApplyCmdLine() {

    if (ApplyUInt32Parameter("-width",  &G_windowWidth))
        G_bufferWidth = G_windowWidth;
    if (ApplyUInt32Parameter("-height", &G_windowHeight))
        G_bufferHeight = G_windowHeight;
    ApplyUInt32Parameter("-samples",    &G_samplesPerPixel);
    ApplyUInt32Parameter("-packetsize", &G_packetSize);
    ApplyUInt32Parameter("-threads",    &G_numThreads, 0);
    ApplyUInt32Parameter("-depth",      &G_maxBounces);
    ApplyUInt32Parameter("-scene",      &G_sceneSelect, 0, ENUM_SCENES_MAX - 1);
    ApplyUInt32Parameter("-mode",       &G_threadingMode, 0, 1);

    int printHelp = CheckParm("-help") || CheckParm("-?");
    if (printHelp > 0) {
        printf_s("\nAvailable Parameters:\n\
                 -width -height\n\
                 -samples -depth\n\
                 -threads -packetsize\n\
                 -mode [0-1]\n\
                 -scene [0-%i]\n", ENUM_SCENES_MAX - 1);
        exit(0);
    }
}


////////////////////////////
//          MAIN          //
////////////////////////////

LRESULT CALLBACK MainWndProc(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    LRESULT result = 0;

    switch (uMsg) {
    case WM_LBUTTONUP:
        G_delay = false;
        break;
    case WM_DESTROY:
    case WM_CLOSE: {
        G_isRunning = false; // will exit the program soon(tm)!
    } break;

    default:
        result = DefWindowProc(hWindow, uMsg, wParam, lParam);
    }

    return result;
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

    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);

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
    RECT r = { 0, 0, LONG(G_windowWidth), LONG(G_windowHeight) };
    AdjustWindowRectEx(&r, windowStyle, FALSE, windowStyleEx);

    if ((r.right - r.left) == LONG(G_windowWidth)) { // AdjustWindowRect doesn't seem to work for some window styles
        int32 x_adjust = GetSystemMetrics(SM_CXFIXEDFRAME) * 2;
        int32 y_adjust = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFIXEDFRAME) * 2;
        r.right += x_adjust;
        r.bottom += y_adjust;
    }

    HWND mainWindow = CreateWindowEx(windowStyleEx, windowClass.lpszClassName, "MiniRayTracer", windowStyle, 5, 35,
                                     (r.right - r.left), (r.bottom - r.top), NULL, NULL, hInstance, 0);

    // set stretch mode in case buffer size != window size
    HDC DC = GetDC(mainWindow);      // stretch mode is reset if we release the DC, so we just don't...
    //SetStretchBltMode(DC, HALFTONE); // bicubic-like, blurry. comment out if undesired
    //SetBrushOrgEx(DC, 0, 0, NULL);   // required after setting HALFTONE according to MSDN

    // tells Windows how to interpret our backbuffer
    BITMAPINFO bmpInfo;
    bmpInfo.bmiHeader.biSize = sizeof(bmpInfo.bmiHeader);
    bmpInfo.bmiHeader.biWidth = G_bufferWidth;
    bmpInfo.bmiHeader.biHeight = G_bufferHeight; // y axis up
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biBitCount = 32;
    bmpInfo.bmiHeader.biCompression = BI_RGB;

    G_backBuffer = (uint32*) calloc(G_bufferWidth * G_bufferHeight, 4);

    if (G_threadingMode == 1) {
        G_linearBackBuffer = (vec3*) calloc(G_bufferWidth * G_bufferHeight, sizeof(*G_linearBackBuffer));
    }

    /////////////////////////
    // --- Setup Scene --- //
    /////////////////////////

    SetWindowTextA(mainWindow, "MiniRayTracer - Generating Scene...");

    // start timer for scene generation
    LARGE_INTEGER t1_gen;
    QueryPerformanceCounter(&t1_gen);

    scene scene = select_scene((scenes) G_sceneSelect);

    // stop timer, display in window title
    LARGE_INTEGER t2_gen;
    QueryPerformanceCounter(&t2_gen);

    char windowTitle[64];
    sprintf_s(windowTitle, 64, "MiniRayTracer - Scene: %.0fms", 1000.f * float(t2_gen.QuadPart - t1_gen.QuadPart) / freq.QuadPart);
    SetWindowTextA(mainWindow, windowTitle);

    // setup sample distribution
    // TODO: distribution for non-square numbers
    //       unbiased distribution that converges earlier: Sobol sequence or others, see http://woo4.me/wootracer/2d-samplers/
    int32 sqrt_samples = (int32) sqrt((float) G_samplesPerPixel);
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

    /////////////////////////////
    // --- Multi-Threading --- //
    /////////////////////////////

    if (G_numThreads == 0) {
        // ALL YOUR PROCESSOR ARE BELONG TO US!
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        G_numThreads = sysInfo.dwNumberOfProcessors;
        if (G_numThreads < 1) // just in case...
            G_numThreads = 1;
    }

    // setup function arguments for the worker threads

    typedef unsigned int(_stdcall *thread_fn)(void*);
    thread_fn thread_fun;
    void *threadArgs;
    size_t argSize;

    uint32 totalWork = 0;
    G_workDoneCounter = (uint32*) calloc(G_numThreads, sizeof(uint32));

    if (G_threadingMode == 0) {
        thread_fun = draw;
        work_queue *queue = new work_queue(G_bufferWidth, G_bufferHeight, G_packetSize, &totalWork);

        drawArgs *threadArgs_ = (drawArgs*) calloc(G_numThreads, sizeof(drawArgs));
        for (uint32 i = 0; i < G_numThreads; i++)
        {
            // thread arguments are all the same for now
            threadArgs_[i].initstate = (uint64(rand32()) << 32) | rand32();
            threadArgs_[i].initseq = (uint64(rand32()) << 32) | rand32();
            threadArgs_[i].queue = queue;
            threadArgs_[i].scene = scene;
            threadArgs_[i].sample_dist = sample_dist;
            threadArgs_[i].numSamples = numSamples;
            threadArgs_[i].threadId = i;
        }
        threadArgs = (void*) threadArgs_;
        argSize = sizeof(drawArgs);
    }
    else {
        thread_fun = draw2;
        work_queue_multi *queue = new work_queue_multi(G_bufferWidth, G_bufferHeight, G_packetSize, G_numThreads, numSamples, &totalWork);

        draw2Args *threadArgs_ = (draw2Args*) calloc(G_numThreads, sizeof(draw2Args));
        for (uint32 i = 0; i < G_numThreads; i++)
        {
            // thread arguments are all the same for now
            threadArgs_[i].initstate = (uint64(rand32()) << 32) | rand32();
            threadArgs_[i].initseq = (uint64(rand32()) << 32) | rand32();
            threadArgs_[i].queue = queue;
            threadArgs_[i].scene = scene;
            threadArgs_[i].sample_dist = sample_dist;
            threadArgs_[i].numSamples = numSamples;
            threadArgs_[i].threadId = i;
        }
        threadArgs = (void*) threadArgs_;
        argSize = sizeof(draw2Args);
    }


    // delayed start for recording
    PatBlt(DC, 0, 0, G_windowWidth, G_windowHeight, BLACKNESS);

    while (G_delay && G_isRunning) {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Sleep(33);
    }

    // start time for ray tracer
    LARGE_INTEGER t1_trace;
    QueryPerformanceCounter(&t1_trace);

    // start worker threads
    HANDLE *threads = (HANDLE*) calloc(G_numThreads, sizeof(*threads));
    for (uint32 i = 0; i < G_numThreads; i++)
    {
        void* args = (void*) ((uint8*) threadArgs + argSize*i);
        threads[i] = (HANDLE) _beginthreadex(NULL, 0, thread_fun, args, 0, NULL);
    }

    static int32 updateFreq = 60;
    bool updateWindowTitle = true;
    // main loop, draws current picture in window
    while (G_isRunning) {

        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        Sleep(1000 / updateFreq);

        if (updateWindowTitle) {
            // display elapsed time in window title
            LARGE_INTEGER t2_trace;
            QueryPerformanceCounter(&t2_trace);

            uint32 work_done = 0;
            for (uint32 i = 0; i < G_numThreads; i++) {
                work_done += G_workDoneCounter[i];
            }
            float secondsElapsed = float(t2_trace.QuadPart - t1_trace.QuadPart) / freq.QuadPart;
            float pctDone = (100.0f * work_done) / totalWork;
            float ETA = secondsElapsed * (100 / pctDone) - secondsElapsed;

            char frameInfo[128];
            sprintf_s(frameInfo, 128, "%s - Trace: %.2fs (%.0f%% - ETA %.0fs)", windowTitle, secondsElapsed, pctDone, ETA);
            SetWindowTextA(mainWindow, frameInfo);

            if (work_done == totalWork) { // ray tracer is done!
                updateWindowTitle = false;
                updateFreq = 30;
            }
        }
        
        // draw current backbuffer to window
        StretchDIBits(DC, 0, 0, G_windowWidth, G_windowHeight, 0, 0, G_bufferWidth, G_bufferHeight, G_backBuffer, &bmpInfo, DIB_RGB_COLORS, SRCCOPY);
    }

    ReleaseDC(mainWindow, DC);

    // wait for threads to finish, but terminate forcefully if too slow
    DWORD waitResult = WaitForMultipleObjects(G_numThreads, threads, TRUE, 500);
    if (waitResult == WAIT_TIMEOUT) {
        for (uint32 i = 0; i < G_numThreads; i++)
        {
            TerminateThread(threads[i], 1);
        }
    }
    for (uint32 i = 0; i < G_numThreads; i++)
    {
        CloseHandle(threads[i]);
    }

    return 0;
}