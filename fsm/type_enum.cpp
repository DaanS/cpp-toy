#ifndef TYPE_ENUM_HPP
#define TYPE_ENUM_HPP

#include <tuple>
#include <type_traits>
#include <iostream>

struct state_stopped {};
struct state_started {};

struct event_start {};
struct event_stop  {};

void start(event_start const& e) { };
void stop (event_stop  const& e) { };

template<typename Event>
using action_f = void(*)(Event const&);

template<typename InState, typename Event, typename OutState, void(Action)(Event const&)>
struct row {
    using in_state = InState;
    using out_state = OutState;
    using event = Event;
    
    action_f<Event> action = Action;
};

template<typename T> struct extract_in_state  { using type = typename T::in_state ; };
template<typename T> struct extract_out_state { using type = typename T::out_state; };

template<template<typename> class Extractor, typename ...States, typename Row, typename ...Rows> 
constexpr auto state_pack_impl(std::tuple<States...>, Row, Rows&& ...rows) {
    return state_pack_impl<Extractor>(std::tuple<States..., typename Extractor<Row>::type>(), rows...);
}

template<template<typename> class  Extractor, typename ...States>
constexpr auto state_pack_impl(std::tuple<States...> s) {
    return s;
}

template<typename ...Rows>
constexpr auto state_pack() {
    return state_pack_impl<extract_out_state>(state_pack_impl<extract_in_state>(std::tuple<>(), Rows()...), Rows()...);
}

template<typename ...Rows>
struct table {

};

using namespace std;

template<typename T, typename U> constexpr bool
is_same_v = is_same<T, U>::value;

int main () { 
    auto tab = table<
        row<state_stopped, event_start, state_started, &start>,
        row<state_started, event_stop , state_stopped, &stop >
    >();

    constexpr auto tup = state_pack<
        row<state_stopped, event_start, state_started, &start>,
        row<state_started, event_stop , state_stopped, &stop >
    >();

    cout << is_same_v<state_stopped, remove_const_t<tuple_element_t<0, decltype(tup)>>> << endl;
    cout << is_same_v<state_started, remove_const_t<tuple_element_t<1, decltype(tup)>>> << endl;
    cout << is_same_v<state_started, remove_const_t<tuple_element_t<2, decltype(tup)>>> << endl;
    cout << is_same_v<state_stopped, remove_const_t<tuple_element_t<3, decltype(tup)>>> << endl;
}

#endif
