#ifndef CRTP_PIPE_H
#define CRTP_PIPE_H

#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <array>

#include "crtp_source.h"
#include "crtp_sink.h"

namespace ygg {

template<typename WordType, typename Derived>
class pipe : public source<WordType, Derived>, public sink<WordType, Derived> {};

template<typename WordType, size_t Capacity>
class fixed_pipe : public pipe<WordType, fixed_pipe<WordType, Capacity>> {
    std::array<WordType, Capacity> buf;
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

        size_t put(WordType* src, size_t n) {
            std::unique_lock<std::mutex> guard(buf_mutex);
            cv.wait(guard, [this]{return (this->write_idx < Capacity) || stopped;});

            size_t count = std::min(Capacity - write_idx, n);
            std::copy(src, src + count, buf.data() + write_idx);
            write_idx += count;

            guard.unlock();
            cv.notify_one();

            return count;
        }

        size_t get(WordType* dst, size_t n) {
            std::unique_lock<std::mutex> guard(buf_mutex);
            cv.wait(guard, [this]{return (this->read_idx < this->write_idx) || stopped;});

            size_t count = std::min(write_idx - read_idx, n);
            std::copy(buf.data() + read_idx, buf.data() + read_idx + count, dst);
            read_idx += count;
            if (read_idx == write_idx) read_idx = write_idx = 0;

            guard.unlock();
            cv.notify_one();

            return count;
        }

        void stop() {
            stopped = true;
            cv.notify_all();
        }
};

}

#endif
