#ifndef SOURCE_H
#define SOURCE_H

#include <ios>
#include <fstream>
#include <random>
#include <limits>
#include <cstring>

namespace ygg {

class source {
    public:
        virtual size_t get(char* dst, std::streamsize n) = 0;
        virtual void stop() = 0;
};

class file_source : public source {
    std::ifstream ifs;

    public:
        file_source(char const* filename) {
            ifs.open(filename, std::ios_base::in | std::ios_base::binary);
        }

        virtual size_t get(char* dst, std::streamsize n) override {
            ifs.read(dst, n);
            size_t count = ifs.gcount();

            if (ifs.eof()) {
                ifs.clear();
                ifs.seekg(0);
            }

            return count;
        }

        virtual void stop() override {}
};

class random_source : public source {
    std::random_device seed;
    //std::independent_bits_engine<std::mt19937_64, 64, uint64_t> gen;
    //std::mt19937_64 gen;
    std::subtract_with_carry_engine<uint_fast64_t, 64, 5, 16> gen;

    public:
        random_source() :
            gen(seed()) {}

        virtual size_t get(char* dst, std::streamsize n) override {
            uint64_t* d = reinterpret_cast<uint64_t*>(dst);
            for (size_t i = 0; i < n / 8; i++) {
                d[i] = gen();
            }
            return n;
        }

        virtual void stop() override {}
};

template<size_t Capacity>
class random_buf_source : public source {
    std::random_device seed;
    std::mt19937_64 gen;
    std::array<char, Capacity> buf;

    public:
        random_buf_source()
        {
            uint64_t* d = reinterpret_cast<uint64_t*>(buf.data());
            for (size_t i = 0; i < Capacity / 8; i++) {
                d[i] = gen();
            }
        }

        virtual size_t get(char* dst, std::streamsize n) override {
            std::streamsize count = n;
            while (count >= Capacity) {
                std::memcpy(dst, buf.data(), Capacity);
                count -= Capacity;
            }

            std::memcpy(dst, buf.data(), count);

            return n;
        }

        virtual void stop() override {}
};

}

#endif
