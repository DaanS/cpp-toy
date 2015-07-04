#include <iostream>
#include <utility>
#include <cmath>

#include "param-pack.hpp"
#include "symath.hpp"

using namespace pp;
using namespace symath;

namespace test {

double test(double d) { return d; }

} // namespace

using std::cout;
using std::endl;

using namespace test;

int main() {
    cout << std::is_same<int, typename element<0, int, double>::type>::value << endl;
    cout << std::is_same<double, typename element<1, int, double>::type>::value << endl;

    //fails properly
    //cout << std::is_same<double, typename element<2, int, double>::type>::value << endl;

    cout << get<0>(1, 2.0) << endl;
    cout << get<1>(1, 2.1) << endl;

    //fails properly
    //cout << get<2>(1, 2.1) << endl;

    cout << partial_call<var, 0, 1>(4.0, 9.0) << endl;
    cout << partial_call<var, 0>(4.0, 9.0) << endl;

    cout << function_traits<decltype(&var::operator())>::arity << endl;

    cout << sqrt<int_ct<4>>()() << endl;
    cout << sqrt<var>()(9.0) << endl;

    cout << pow<int_ct<2>, int_ct<3>>()() << endl;
    cout << pow<var, int_ct<3>>()(2.0) << endl;
    cout << pow<int_ct<2>, var>()(3.0) << endl;
    cout << pow<var, var>()(2.0, 3.0) << endl;
    cout << pow<sqrt<var>, int_ct<2>>()(2.1) << endl;

    cout << sum(1, 2, 3) << endl;

    cout << sin<var>()(1.57) << endl;

    cout << partial_sum<0>(1, 2, 3) << endl;
    cout << partial_sum<1>(1, 2, 3) << endl;
    cout << partial_sum<2>(1, 2, 3) << endl;
    cout << partial_sum<3>(1, 2, 3) << endl;
}
