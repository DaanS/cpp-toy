#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

//#include "benchmark.h"
#include "source.h"
#include "sink.h"
#include "pipe.h"

namespace ygg {

using clock = std::chrono::high_resolution_clock;

struct data_point {
    clock::time_point time;
    uint_fast64_t count;
};

template<size_t Capacity>
class fixed_worker {
    std::array<char, Capacity> buf;
    std::shared_ptr<source> src;
    std::shared_ptr<sink> dst;
    bool stopped;
    std::atomic_uint_fast64_t bytes_written;

    public:
        fixed_worker(std::shared_ptr<source> src, std::shared_ptr<sink> dst) : 
            src(src), 
            dst(dst), 
            stopped(false),
            bytes_written(0) {}

        void work(std::string name) {
            //std::cout << "Worker " << name << " starting..." << std::endl;

            while (!stopped) {
                size_t count = src->get(buf.data(), Capacity);

                size_t write_idx = 0;
                while (write_idx < count && !stopped) {
                    write_idx += dst->put(buf.data() + write_idx, count - write_idx);       
                }
                bytes_written += count;
            }

            //std::cout << "Worker " << name << " done." << std::endl;
        }

        void poll(data_point& data) {
            data.time = clock::now();
            data.count = bytes_written;
        }

        void stop() {
            stopped = true;
            src->stop();
            dst->stop();
        }
};

}

using namespace ygg;
namespace po = boost::program_options;

constexpr size_t sample_count = 10;

int main(int argc, char* argv[]) {
    std::string input_file;
    std::string output_file;
    int buffer_size;

    // CLI
    po::options_description desc("Supported options");
    desc.add_options()
        ("help", "produce help message")
        ("input-file", po::value<std::string>(&input_file)->default_value("512k.dat"), "input file")
        ("output-file", po::value<std::string>(&output_file)->default_value("out.dat"), "output file")
        ("buffer-size", po::value<int>(&buffer_size)->default_value(1024), "buffer size");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) std::cout << desc << std::endl;

    // Set up pipeline
    auto src = std::make_shared<random_buf_source<1*1024*1024> >();
    auto dst = std::make_shared<null_sink>();
    auto pipe = std::make_shared<fixed_pipe<1*1024*1024> >();

    auto w1 = std::make_shared<fixed_worker<1*256*1024> >(src, pipe);
    auto w2 = std::make_shared<fixed_worker<1*256*1024> >(pipe, dst);

    std::vector<data_point> d1(sample_count);
    std::vector<data_point> d2(sample_count);

    // Run
    std::thread t1([w1] {w1->work("w1");});
    std::thread t2([w2] {w2->work("w2");});
    
    // Poll
    clock::time_point start = clock::now();

    for (size_t i = 0; i < sample_count; i++) {
        std::this_thread::sleep_until(start + std::chrono::seconds(i + 1));
        w1->poll(d1[i]);
        w2->poll(d2[i]);
    }

    w1->stop();
    w2->stop();

    // Process
    for (size_t i = 0; i < sample_count; i++) {
        auto dur1 = std::chrono::duration_cast<std::chrono::microseconds>(d1[i].time - start).count();
        auto dur2 = std::chrono::duration_cast<std::chrono::microseconds>(d2[i].time - start).count();

        size_t delta1 = 0;
        size_t delta2 = 0;

        if (i > 0) {
            delta1 = (d1[i].count - d1[i - 1].count) / 1024;       
            delta2 = (d2[i].count - d2[i - 1].count) / 1024;       
        }

        printf("%8ld us: %11ld (%8ld KiB/s), %8ld us: %11ld (%8ld KiB/s)\n", dur1, d1[i].count, delta1, dur2, d2[i].count, delta2);
    }

    t1.join();
    t2.join();
}
