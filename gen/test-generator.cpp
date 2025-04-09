#include <iostream>
#include <array>
#include <utility>

namespace gen {

template<typename T, std::size_t ...Ns>
constexpr std::array<T, sizeof...(Ns)> make_array(std::integer_sequence<T, Ns...>) {
    return {Ns...};
}

template<typename Op>
struct pipeable {
    template<typename SrcGen>
    struct generator {
        SrcGen gen;
        Op op;

        template<typename Iterator>
        struct iterator_impl {
            using self = iterator_impl<Iterator>;

            Iterator it;
            Iterator end;
            size_t yield;
            Op op;

            self& operator++() { ++yield; return *this; }
            decltype(auto) operator*() { decltype(auto) res = op(it, end, yield); yield = 0; return res; }
            bool operator==(self& rhs) { this->operator*(); rhs.operator*(); return it == rhs.it; }
            bool operator!=(self& rhs) { return !(*this == rhs); }
        };

        using src_gen_type = typename std::remove_reference<SrcGen>::type;
        using iterator = iterator_impl<typename src_gen_type::iterator>;
        using const_iterator = iterator_impl<typename src_gen_type::const_iterator>;

        template<typename Iterator>
        constexpr auto make_iterator(Iterator it, Iterator end) const { return iterator_impl<Iterator>{it, end, 0, op}; }

        auto begin() { return make_iterator(gen.begin(), gen.end()); }
        auto end() { return make_iterator(gen.end(), gen.end()); }
        constexpr auto cbegin() const { return make_iterator(gen.cbegin(), gen.cend()); }
        constexpr auto cend() const { return make_iterator(gen.cend(), gen.cend()); }
        constexpr auto begin() const { return make_iterator(gen.cbegin(), gen.cend()); }
        constexpr auto end() const { return make_iterator(gen.cend(), gen.cend()); }
    };

    template<typename Generator>
    constexpr static generator<Generator> pipe(Generator&& gen, Op&& p) {
        return generator<Generator>{std::forward<Generator>(gen), std::forward<Op>(p)};
    }
};

template<typename Generator, typename Pipeable>
constexpr auto operator|(Generator&& gen, Pipeable&& p) { return Pipeable::pipe(std::forward<Generator>(gen), std::forward<Pipeable>(p)); }

template<typename Pred>
struct where_impl : pipeable<where_impl<Pred>> {
    Pred const pred;
    constexpr where_impl(Pred pred) : pred(pred) { }

    template<typename Iterator>
    constexpr decltype(auto) operator()(Iterator& it, Iterator const& end, size_t n) const {
        for (size_t i = 0; i < n; ++i) {
            do { ++it; } while (!pred(*it));
        }
        return *it;
    }
};
template<typename Pred> constexpr where_impl<Pred> where(Pred pred) { return {pred}; }

template<typename Fun>
struct select_impl : pipeable<select_impl<Fun>> {
    Fun const fun;
    constexpr select_impl(Fun fun) : fun(fun) { }

    template<typename Iterator>
    constexpr decltype(auto) operator()(Iterator& it, Iterator const& end, size_t n) const {
        for (size_t i = 0; i < n; ++i) ++it;
        return fun(*it);
    }
};
template<typename Fun> constexpr select_impl<Fun> select(Fun fun) { return {fun}; }

}

struct number {
    int value;
    bool is_even = false;

    number(int value) : value(value) { }
};

bool is_even(int n) { return n % 2 == 0; }
int times_hundred(int n) { return n * 100; }

struct is_even_functor {
    constexpr is_even_functor() {}
    constexpr bool operator()(int n) const { return n % 2 == 0; }
};

template<typename T>
void print_type_traits(T&& t) {
    std::cout << "const: " << std::is_const<T>::value << std::endl;
    std::cout << "ref:   " << std::is_reference<T>::value << std::endl;
}

template<typename T>
constexpr decltype(auto) first_element(T&& t) {
    return t[0];
}

using gen::select;
using gen::where;
using gen::make_array;

int main() {
    constexpr static std::array<size_t, 11> integers = make_array(std::make_index_sequence<11>());
    std::cout << std::integral_constant<int, first_element(integers)>::value << std::endl;
    constexpr is_even_functor functor;
    constexpr auto tmp = where(functor);
    //constexpr auto res_const = operator|(integers, where(functor));
    print_type_traits(integers);
    constexpr auto res_const = integers
                             | where(is_even)
                             | gen::select(times_hundred);
    for (auto n : res_const) std::cout << n << std::endl;

    auto res = integers
             | where([](int n) { return n % 2 == 0; })
             | gen::select([](int n) { return n * 100; });
    for (auto n : res) std::cout << n << std::endl;

    std::array<number, 11> numbers{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto res2 = numbers
              | where([](number n) { return n.value % 2 == 0; })
              | gen::select([](number& n) -> bool& { return n.is_even; });
    for (bool& n : res2) n = true;
    for (auto n : numbers) std::cout << n.value << " is " << (n.is_even ? "even" : "odd") << std::endl;

    auto test = std::array<size_t, 11>() | where([](int n) { return true; });
    std::cout << std::is_same<std::remove_const<decltype(integers)>::type, decltype(test.gen)>::value << std::endl;
}
