#ifndef CRTP_SINK_H
#define CRTP_SINK_H

#include <ios>
#include <fstream>

namespace ygg {

template<typename WordType, typename Derived>
class sink {
    private:
        Derived& derived() { return *static_cast<Derived*>(this); }

    public:
        size_t put(WordType* src, size_t n) { return derived().put(src, n); }
        void stop() { derived().stop(); }
};

template<typename WordType>
class file_sink : public sink<WordType, file_sink<WordType>> {
    std::ofstream ofs;

    public:
        file_sink(char const* filename) {
            ofs.open(filename, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
        }

        size_t put(WordType* src, size_t n) {
            ofs.write(reinterpret_cast<char*>(src), n);
            return n;
        }

        void stop() {}
};

template<typename WordType>
class null_sink : public sink<WordType, null_sink<WordType>> {
    public:
        size_t put(WordType* src, size_t n) { return n; }
        void stop() {}
};

}

#endif
