#ifndef SINK_H
#define SINK_H

#include <ios>
#include <fstream>

namespace ygg {

class sink { 
    public: 
        virtual size_t put(char* src, std::streamsize n) = 0; 
        virtual void stop() = 0;
};

class file_sink : public sink {
    std::ofstream ofs;

    public:
        file_sink(char const* filename) {
            ofs.open(filename, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
        }

        virtual size_t put(char* src, std::streamsize n) override {
            ofs.write(src, n);
            return n;
        }

        virtual void stop() override {}
};

class null_sink : public sink {
    public:
        virtual size_t put(char* src, std::streamsize n) override { return n; }
        virtual void stop() override {}
};

}

#endif
