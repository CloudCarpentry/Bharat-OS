#include "bench.h"
#include "bench_runner.h"

// Define sections for linker to place registered benchmarks
#ifdef __APPLE__
    #define SECTION_NAME "__DATA,benchmarks"
#else
    #define SECTION_NAME "benchmarks"
#endif

// We use GNU __start/__stop attributes via linker script or compiler features
// For host tests, these will be populated correctly if the compiler supports GCC __attribute__((used, section("benchmarks"))).
// If not, we fall back to a manual array registration approach.
