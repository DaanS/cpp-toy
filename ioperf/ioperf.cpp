#include <cstring>
#include <ios>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <algorithm>
#include <condition_variable>
#include <thread>
#include <chrono>

#include "benchmark.h"

class source { public: virtual size_t get(char* dst, std::streamsize n) = 0; };
class sink   { public: virtual size_t put(char* src, std::streamsize n) = 0; };

class pipe : public source, public sink {};

class file_source : public source {
    std::ifstream ifs;

    public:
        file_source(char const* filename) {
            ifs.open(filename, std::ios_base::in | std::ios_base::binary);
        }

        virtual size_t get(char* dst, std::streamsize n) override {
            ifs.read(dst, n);
            return ifs.gcount();
        }
};

class file_sink : public sink {
    std::ofstream ofs;

    public:
        file_sink(char const* filename) {
            ofs.open(filename, std::ios_base::out | std::ios_base::binary);
        }

        virtual size_t put(char* src, std::streamsize n) override {
            ofs.write(src, n);
            return n;
        }
};

class array_pipe : public pipe {
    char* buf;
    size_t capacity;
    size_t write_idx;
    size_t read_idx;
    std::condition_variable cv;
    std::mutex buf_mutex;

    public:
        array_pipe(size_t capacity) : capacity(capacity), write_idx(0), read_idx(0), buf(new char[capacity]) {}
        virtual ~array_pipe() { delete buf; }

        virtual size_t put(char* src, std::streamsize n) override {
            std::unique_lock<std::mutex> guard(buf_mutex);
            cv.wait(guard, [this]{return this->write_idx < this->capacity;});

            size_t count = std::min<long>(capacity - write_idx, n);
            std::memcpy(buf + write_idx, src, count);
            write_idx += count;
            
            guard.unlock();
            cv.notify_one();

            return count;
        }

        virtual size_t get(char* dst, std::streamsize n) override {
            std::unique_lock<std::mutex> guard(buf_mutex);
            cv.wait_for(guard, std::chrono::seconds(1), [this]{return this->read_idx < this->write_idx;});

            size_t count = std::min<long>(write_idx - read_idx, n);
            std::memcpy(dst, buf + read_idx, count);
            read_idx += count;
            if (read_idx == write_idx) read_idx = write_idx = 0;

            guard.unlock();
            cv.notify_one();

            return count;
        }
};

class worker {
    std::shared_ptr<source> src;
    std::shared_ptr<sink> dst;
    char* buf;
    size_t capacity;

    public:
        worker(std::shared_ptr<source> src, std::shared_ptr<sink> dst, size_t capacity) : src(src), dst(dst), capacity(capacity), buf(new char[capacity]) {}

        void work() {
            std::cout << "Worker starting..." << std::endl;
            while (true) {
                size_t count = src->get(buf, capacity);
                if (count == 0) break;

                size_t write_idx = 0;
                while (write_idx < count) {
                    write_idx += dst->put(buf + write_idx, count - write_idx);       
                }
            }
            std::cout << "Worker done." << std::endl;
        }
};

void do_work(std::shared_ptr<source> src, std::shared_ptr<sink> dst, size_t worker_capacity) {
    worker w(src, dst, worker_capacity);
    w.work();
}

int main(int argc, char* argv[]) {
    for (int i = 0; i < argc; i++) std::cout << argv[i] << std::endl;

    std::shared_ptr<source> src = std::make_shared<file_source>("512k.dat");
    std::shared_ptr<sink> dst = std::make_shared<file_sink>("out.dat");
    std::shared_ptr<pipe> pipe = std::make_shared<array_pipe>(4);

    ygg::benchmark::start();
    std::thread t1(do_work, src, pipe, 4);
    std::thread t2(do_work, pipe, dst, 4);
    t1.join();
    t2.join();
    ygg::benchmark::end();
}
