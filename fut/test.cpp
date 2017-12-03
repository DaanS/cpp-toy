#include <iostream>
#include <random>
#include <algorithm>

#include "fut.h"
#include "benchmark.h"

constexpr size_t total_samples = 1'000'000;
constexpr size_t total_jobs = 4;

using age_type = uint_fast16_t;

age_type sum_aging(size_t n_samples) {
    std::random_device r;
    std::default_random_engine e(r());
    std::uniform_int_distribution<age_type> d(1, 100);

    age_type res = 0;

    for (size_t i = 0; i < n_samples; ++i) {
        age_type age = 0;
        while (d(e) > age) ++ age;
        res += age;
    }

    return res;
}

int main() {
    std::cout << fut::async(fut::launch::sync, sum_aging, 100).get() << std::endl;

    using sum_aging_future = std::future<fut::async_result<decltype(sum_aging), size_t>>;

    benchmark::start();

    std::vector<sum_aging_future> res;
    for (size_t i = 0; i < total_jobs; i++) res.push_back(std::async(std::launch::async, sum_aging, total_samples / total_jobs));
    std::cout << "Mean: " << std::accumulate(res.begin(), res.end(), 0, [](age_type i, sum_aging_future& f) { return i + f.get(); }) / (float) total_samples << std::endl;

    benchmark::end();
}
