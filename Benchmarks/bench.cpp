#include "bench.h"
#include "pcg.h"
#include "main.h"
#include "cmdline_parser.h"
#include <thread>

// dummies to make the linker happy
void MRT::MouseCallback(int32 x, int32 y, KeyState lButton, KeyState rButton) {}
void MRT::KeyboardCallback(int keycode, KeyState state, KeyState prev) {}
void MRT::WindowCallback(WindowEvent e) {}

// NOTE: unfortunately, this cannot be set after dynamic initialization. benchmark runs x times faster.
uint32 threads = std::thread::hardware_concurrency() / 2;

int main(int argc, char *argv[]) {
    printf("NOTE: use --benchmark_filter=<regex> to select specific benchmarks (e.g. \"Mat4.*\")\n\n");
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    return 0;
}