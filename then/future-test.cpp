#include <iostream>
#include <memory>

#include "future.h"

struct no_copy {
    no_copy() {}
    no_copy(no_copy const & other) = delete;
    no_copy(no_copy && other) {}

    no_copy& operator=(no_copy const & other) = delete;
    no_copy& operator=(no_copy && other) { return *this; }
    friend std::ostream& operator<<(std::ostream& os, no_copy const & nc);
};

std::ostream& operator<<(std::ostream& os, no_copy const & nc) {
    return os << "no_copy";
}

struct no_move {
    no_move() {}
    no_move(no_move const & other) {}
    //no_move(no_move && other) = delete;
    ~no_move() {}

    no_move& operator=(no_move const& other) { return *this; }
    no_move& operator=(no_move && other) = delete;
    friend std::ostream& operator<<(std::ostream& os, no_move const & nc);
};

std::ostream& operator<<(std::ostream& os, no_move const & nc) {
    return os << "no_move";
}

int main() {
    using Type = no_move;
    std::shared_ptr<then::shared_state<Type>> state = std::make_shared<then::shared_state<Type>>();
    then::future<Type> fut;
    fut.set(state);

    state->set_value(Type());
    std::cout << fut.get() << std::endl;
}
