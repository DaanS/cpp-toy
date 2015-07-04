#ifndef FSM_HPP
#define FSM_HPP

#include <iostream>

template<typename State>
struct state {

};

struct state_stopped : state<state_stopped> {};
struct state_started : state<state_started> {};

struct event_start{};
struct event_stop {};

void start(event_start const& e) { std::cout << "Starting..." << std::endl; }
void stop (event_stop  const& e) { std::cout << "Stopping..." << std::endl; }

template<typename InState, typename Event, typename OutState, void(*Action)(Event const&)>
struct transition {

};


template<typename State, typename ...Transitions>
struct state_machine_impl{};

template<typename ...Transitions>
struct state_machine {

};

template<typename derived>
struct crtp_base {
    void foo() { impl().foo(); }
    derived& impl() { return *static_cast<derived*>(this); }
};

struct crtp_deriv : crtp_base<crtp_deriv> {
    void foo() { std::cout << "foo" << std::endl; }
};

int main() {
    auto fsm = state_machine<
        transition<state_stopped, event_start, state_started, &start>,
        transition<state_started, event_stop, state_stopped, &stop>
    >();
}

#endif
