#ifndef SYMATH_HPP
#define SYMATH_HPP

#include <cmath>

#include "param-pack.hpp"

namespace symath {

template<typename T, T I>
struct integral_const {
    enum { arity = 0 };

    constexpr T operator()() { return I; }
};

template<int I>
using int_ct = integral_const<int, I>;

struct var {
    enum { arity = 1 };

    constexpr double operator()(double d) { return d; }
};

template<double(F)(double), typename T>
struct unary_f {
    enum { arity = T::arity };

    template<typename... Args>
    constexpr double operator()(Args&&... args) { return F(pp::partial_call<T, 0>(std::forward<Args>(args)...)); }
};

template<double(F)(double, double), typename T, typename U>
struct binary_f {
    enum { arity = T::arity + U::arity };

    template<typename... Args>
    constexpr double operator()(Args&&... args) { return F(pp::partial_call<T, 0>(std::forward<Args>(args)...),
                                                           pp::partial_call<U, T::arity>(std::forward<Args>(args)...)); }
};

template<typename F_type, F_type F, typename ...Ts>
struct anary_f {
    enum { arity = pp::sum(Ts::arity...) };

    template<typename... Args>
    constexpr double operator()(Args&&... args) { return pp::multi_call<F_type, F, Ts...>(std::forward<Args>(args)...); }
};

template<typename T>
struct sin : anary_f<double(double), std::sin, T> { };

template<typename T>
struct sqrt : unary_f<std::sqrt, T> { };

template<typename T, typename U>
struct pow : binary_f<std::pow, T, U> { };

template<typename T>
struct pow<sqrt<T>, int_ct<2>> : T { };

} // namespace

#endif
