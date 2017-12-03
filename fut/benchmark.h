#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <chrono>
#include <cstdio>

namespace benchmark {
    using clock = std::chrono::high_resolution_clock;
    using duration_type = std::chrono::nanoseconds;

    clock::time_point start_time;

    void start() { start_time = clock::now(); }

    duration_type end_silent() { return std::chrono::duration_cast<duration_type>(clock::now() - start_time); }

    duration_type end() {
        duration_type dur = end_silent();

        std::printf("Total time: %12ld ns\n", dur.count());

        return dur;
    }
}

#endif
