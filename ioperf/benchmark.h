#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <chrono>
#include <stdio.h>

namespace ygg {

struct benchmark {
    using clock = std::chrono::high_resolution_clock;
    static clock::time_point start_time;

    static void start() {
        start_time = clock::now();
    }

    static long end() {
        auto end = clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - start_time);

        printf("Total time: %9ld us\n", dur.count());

        return dur.count();
    }

    static long dur(clock::time_point start, clock::time_point end) {
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }
};

benchmark::clock::time_point benchmark::start_time;

}

#endif
