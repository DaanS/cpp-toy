#ifndef CRTP_SOURCE_H
#define CRTP_SOURCE_H

#include <ios>
#include <fstream>
#include <random>
#include <array>
#include <cstring>

namespace ygg {

template<typename WordType, typename Derived>
class source {
    private:
        Derived& derived() { return *static_cast<Derived*>(this); }
    
    public:
        size_t get(WordType* dst, size_t n) { derived().get(dst, n); }
        void stop() { derived().stop(); }
};

template<typename WordType>
class file_source : public source<WordType, file_source<WordType>> {
    std::ifstream ifs;

    public:
        file_source(char const* filename) {
            ifs.open(filename, std::ios_base::in | std::ios_base::binary);
        }

        size_t get(WordType* dst, size_t n) {
            ifs.read(reinterpret_cast<char*>(dst), n);
            size_t count = ifs.gcount();

            if (ifs.eof()) {
                ifs.clear();
                ifs.seekg(0);
            }

            return count;
        }

        void stop() {}
};

template<typename WordType, size_t Capacity>
class random_buf_source : public source<WordType, random_buf_source<WordType, Capacity>> {
    std::random_device seed;
    std::independent_bits_engine<std::mt19937_64, sizeof(WordType) * 8, WordType> gen;
    std::array<WordType, Capacity> buf;

    public:
        random_buf_source() {
            for (size_t i = 0; i < Capacity; i++) {
                buf[i] = gen();
            }
        }

        size_t get(WordType* dst, size_t n) {
            size_t count = n;
            while (count >= Capacity) {
                std::memcpy(dst, buf.data(), Capacity);
                count -= Capacity;
            }

            std::memcpy(dst, buf.data(), count);

            return n;
        }

        void stop() {}
};

}

#endif
