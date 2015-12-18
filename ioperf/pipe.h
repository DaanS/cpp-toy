#ifndef PIPE_H
#define PIPE_H

#include <cstring>
#include <cassert>
#include <cstdio>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <array>

#include "source.h"
#include "sink.h"

namespace ygg {

class pipe : public source, public sink {};

template<typename WordType, size_t Capacity>
class sized_pipe : public pipe {
    std::array<WordType, Capacity> buf;
    size_t write_idx;
    size_t read_idx;
    bool stopped;

    std::condition_variable cv;
    std::mutex buf_mutex;

    public:
        sized_pipe() : 
            write_idx(0), 
            read_idx(0), 
            stopped(false) {}

        virtual size_t put(WordType* src, std::streamsize n) override {
            std::unique_lock<std::mutex> guard(buf_mutex);
            cv.wait(guard, [this]{return (this->write_idx < Capacity) || stopped;});

            size_t count = std::min<long>(Capacity - write_idx, n);
            std::copy(src, src + count, buf.data() + write_idx);
            write_idx += count;
            
            guard.unlock();
            cv.notify_one();

            return count;
        }

        virtual size_t get(char* dst, std::streamsize n) override {
            std::unique_lock<std::mutex> guard(buf_mutex);
            cv.wait(guard, [this]{return (this->read_idx < this->write_idx) || stopped;});

            size_t count = std::min<long>(write_idx - read_idx, n);
            std::copy(buf.data() + read_idx, buf.data() + read_idx + count, dst);
            read_idx += count;
            if (read_idx == write_idx) read_idx = write_idx = 0;

            guard.unlock();
            cv.notify_one();

            return count;
        }

        virtual void stop() override {
            stopped = true;
            cv.notify_all();
        }
};

template<size_t Capacity>
class fixed_pipe : public pipe {
    std::array<char, Capacity> buf;
    size_t write_idx;
    size_t read_idx;
    bool stopped;

    std::condition_variable cv;
    std::mutex buf_mutex;

    public:
        fixed_pipe() : 
            write_idx(0), 
            read_idx(0), 
            stopped(false) {}

        virtual size_t put(char* src, std::streamsize n) override {
            std::unique_lock<std::mutex> guard(buf_mutex);
            cv.wait(guard, [this]{return (this->write_idx < Capacity) || stopped;});

            size_t count = std::min<long>(Capacity - write_idx, n);
            std::copy(src, src + count, buf.data() + write_idx);
            write_idx += count;
            
            guard.unlock();
            cv.notify_one();

            return count;
        }

        virtual size_t get(char* dst, std::streamsize n) override {
            std::unique_lock<std::mutex> guard(buf_mutex);
            cv.wait(guard, [this]{return (this->read_idx < this->write_idx) || stopped;});

            size_t count = std::min<long>(write_idx - read_idx, n);
            std::copy(buf.data() + read_idx, buf.data() + read_idx + count, dst);
            read_idx += count;
            if (read_idx == write_idx) read_idx = write_idx = 0;

            guard.unlock();
            cv.notify_one();

            return count;
        }

        virtual void stop() override {
            stopped = true;
            cv.notify_all();
        }
};

class circular_pipe : public pipe {
    char* buf;
    size_t capacity;
    size_t write_idx;
    size_t read_idx;
    bool stopped;

    std::condition_variable cv;
    std::mutex buf_mutex;

    public:
        circular_pipe(size_t capacity) : 
            capacity(capacity), 
            write_idx(0), 
            read_idx(0), 
            buf(new char[capacity]),
            stopped(false) {}
        virtual ~circular_pipe() { delete[] buf; }



        virtual size_t put(char* src, std::streamsize n) override {
            std::unique_lock<std::mutex> guard(buf_mutex);
            cv.wait(guard, [this]{return (this->write_idx + 1 != this->read_idx) || stopped;});

            size_t total_count = 0;

            //printf("put: n(%8ld), write(%8ld), read(%8ld)", n, write_idx, read_idx);

            if (write_idx >= read_idx) {
                size_t count = std::min<size_t>(capacity - write_idx - (read_idx == 0), n);
                assert(write_idx + count <= capacity - (read_idx == 0));
                std::memcpy(buf + write_idx, src, count);
                write_idx += count;
                total_count += count;
                n -= count;

                if (write_idx == capacity) write_idx = 0;
            }

            if (n > 0 && write_idx + 1 < read_idx) {
                size_t count = std::min<size_t>(read_idx - 1 - write_idx, n);
                assert(write_idx + count < capacity);
                std::memcpy(buf + write_idx, src + total_count, count);
                write_idx += count;
                total_count += count;
            }

            //printf(" did %8ld\n", total_count);
            
            guard.unlock();
            cv.notify_one();

            return total_count;
        }

        virtual size_t get(char* dst, std::streamsize n) override {
            std::unique_lock<std::mutex> guard(buf_mutex);
            cv.wait(guard, [this]{return (this->read_idx != this->write_idx) || stopped;});

            size_t total_count = 0;

            if (read_idx > write_idx) {
                size_t count = std::min<size_t>(capacity - read_idx, n);
                std::memcpy(dst, buf + read_idx, count);
                read_idx += count;
                total_count += count;
                n -= count;

                if (read_idx == capacity) read_idx = 0;
            }

            if (n > 0 && read_idx < write_idx) {
                size_t count = std::min<size_t>(write_idx - read_idx, n);
                std::memcpy(dst + total_count, buf + read_idx, count);
                read_idx += count;
                total_count += count;
            }

            //printf("get: %8ld\n", total_count);

            guard.unlock();
            cv.notify_one();

            return total_count;
        }

        virtual void stop() override {
            stopped = true;
            cv.notify_all();
        }
};

}

#endif
