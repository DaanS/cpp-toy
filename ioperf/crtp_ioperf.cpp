#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include "crtp_source.h"
#include "crtp_sink.h"
#include "crtp_pipe.h"

namespace ygg {

using clock = std::chrono::high_resolution_clock;

struct data_point {
    clock::time_point time;
    uint_fast64_t count;
};

template<typename WordType, typename SourceType, typename SinkType, size_t Capacity>
class fixed_worker {
    std::array<WordType, Capacity> buf;
    std::shared_ptr<SourceType> src;
    std::shared_ptr<SinkType> dst;
    bool stopped;
    std::atomic_uint_fast64_t elts_written;

    public:
        fixed_worker(std::shared_ptr<SourceType> src, std::shared_ptr<SinkType> dst) :
            src(src),
            dst(dst),
            stopped(false),
            elts_written(0) {}

        void work() {
            while (!stopped) {
                size_t count = src->get(buf.data(), Capacity);

                size_t write_idx = 0;
                while (write_idx < count && !stopped) {
                    write_idx += dst->put(buf.data() + write_idx, count - write_idx);
                }
                elts_written += count; 
            }
        }

        void poll(data_point& data) {
            data.time = clock::now();
            data.count = elts_written;
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
    using WordType = unsigned char;
    using SourceType = random_buf_source<WordType, 1*1024*1204>;
    using SinkType = null_sink<WordType>;
    using PipeType = fixed_pipe<WordType, 1*1024*1024>;

    auto src = std::make_shared<SourceType>();
    auto dst = std::make_shared<SinkType>();
    auto pipe = std::make_shared<PipeType>();

    auto w1 = std::make_shared<fixed_worker<WordType, SourceType, PipeType, 1*256*1024>>(src, pipe);
    auto w2 = std::make_shared<fixed_worker<WordType, PipeType, SinkType, 1*256*1024> >(pipe, dst);

    std::vector<data_point> d1(sample_count);
    std::vector<data_point> d2(sample_count);

    // Run
    std::thread t1([w1] {w1->work();});
    std::thread t2([w2] {w2->work();});
    
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

        d1[i].count *= sizeof(WordType);
        d2[i].count *= sizeof(WordType);

        if (i > 0) {
            delta1 = (d1[i].count - d1[i - 1].count) / 1024;       
            delta2 = (d2[i].count - d2[i - 1].count) / 1024;       
        }

        printf("%8ld us: %11ld (%8ld KiB/s), %8ld us: %11ld (%8ld KiB/s)\n", dur1, d1[i].count, delta1, dur2, d2[i].count, delta2);
    }

    t1.join();
    t2.join();
}
