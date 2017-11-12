#pragma once

#include "common.h"

// Google Benchmark's header does not compile without warnings...
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4141 )
#endif

    #include "benchmark.h"

#ifdef __clang__
#pragma clang diagnostic pop
#elif _MSC_VER
#pragma warning( pop )
#endif


//#define ENABLE_IACA
#ifdef ENABLE_IACA
    // enable/disable groups of tests to produce a reasonable amount of IACA output
    // NOTE: run with --benchmark_filter=<regex> to filter at runtime
    #define ENABLE_BENCH_VEC3
    #define ENABLE_BENCH_MAT4
#else
    #undef IACA_START
    #undef IACA_END
    #define IACA_START
    #define IACA_END
#endif

extern uint32 threads;

#define BENCHMARK_MRT(name, var) BENCHMARK_TEMPLATE(name, var)->Repetitions(4)->ReportAggregatesOnly()->Threads(threads)
