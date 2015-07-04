#include <cstdio>
#include "symath.hpp"

using namespace symath;

double id(double d) {
    return pow<sqrt<var>, int_ct<2>>()(d);
}

double my_sqrt(double d) {
    return pow<sqrt<var>, int_ct<1>>()(d);
}

int main() {
    std::printf("%f\n", my_sqrt(4.0));
}
