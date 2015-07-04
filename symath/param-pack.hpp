#ifndef PARAMETER_PACK_HPP
#define PARAMETER_PACK_HPP

#include <utility>

namespace pp {

/**
 * \brief Returns a single element of a template parameter pack
 * \tparam N index of the desired element
 * \tparam A head of the template parameter pack
 * \tparam ...Args tail of the template parameter pack
 * \pre N <= sizeof...(Args)
 * \retval ::type the Nth element in the template parameter pack
 * \example using my_element = element<0, int, double>
 */
template<std::size_t N, typename A, typename ...Args>
struct element {
    static_assert(N <= sizeof...(Args), "element: index out of range");
    using type = typename element<N - 1, Args...>::type;
};

// element terminator
template<typename A, typename ...Args>
struct element<0, A, Args...> {
    using type = A;
};

// get

template<std::size_t N, typename A, typename ...Args>
struct get_impl {
    static_assert(N <= sizeof...(Args), "get: index out of range");
    constexpr auto operator()(A arg, Args&&... args) { 
        return get_impl<N - 1, Args...>()(std::forward<Args>(args)...); 
    }
};

template<typename A, typename ...Args>
struct get_impl<0, A, Args...> {
    constexpr auto operator()(A arg, Args&&... args) { 
        return arg; 
    }
};

/**
 * \brief Returns a single element of a function parameter pack
 * \tparam N index of the desired element
 * \param ...args the function parameter pack
 * \return the Nth element in the function parameter pack
 * \example char c = get<0>('a', 'b')
 */
template<std::size_t N, typename ...Args>
constexpr auto get(Args&&... args) {
    static_assert(N < sizeof...(Args), "get: index out of range");
    return get_impl<N, Args...>()(std::forward<Args>(args)...);
}

// function_traits

template<typename F> struct function_traits { };

template<typename R, typename ...Args>
struct function_traits<R(Args...)> {
    enum { arity = sizeof...(Args) };
};

template<typename R, typename T, typename ...Args>
struct function_traits<R(T::*)(Args...)> : function_traits<R(Args...)> { };

template<typename R, typename T, typename ...Args>
struct function_traits<R(T::*)(Args...) const> : function_traits<R(Args...)> { };

// partial_call

template<typename F, std::size_t S, std::size_t... Is, typename... Args>
constexpr auto partial_call_impl(std::index_sequence<Is...>, Args&&... args) {
    return F()(get<Is + S>(std::forward<Args>(args)...)...);
}

template<typename T, typename U = void>
struct get_arity {
    enum { arity = function_traits<decltype(&T::operator())>::arity };
};

template<typename T>
struct get_arity<T, typename std::enable_if<(T::arity >= 0)>::type> {
    enum { arity = T::arity };
};

/**
 * \brief Calls a functor with arguments taken from a function parameter pack
 * \tparam F the functor to call
 * \tparam S index of the first argument in the funtion parameter pack
 * \tparam N the number of arguments to consume from the function parameter pack (defaults to declared arity of F::operator())
 * \param ...args the function parameter pack
 * \pre (N + S) <= sizeof...(args)
 * \return return value of F
 * \example partial_call<MyFunctor, 1, 2>(1.0, 2.0, 3.0) === MyFunctor()(2.0, 3.0)
 */
template<typename F, std::size_t S, std::size_t N = get_arity<F>::arity, typename... Args>
constexpr auto partial_call(Args&&... args) {
    static_assert(N + S <= sizeof...(args), "partial_call: insufficient arguments");
    return partial_call_impl<F, S>(std::make_index_sequence<N>(), std::forward<Args>(args)...);   
}

// sum

constexpr auto sum() { return 0; }

template<typename T, typename ...Ts>
constexpr auto sum(T&& t, Ts&&... ts) { return t + sum(std::forward<Ts>(ts)...); }

// partial_sum

template<std::size_t N, std::size_t ...Is, typename ...Args>
constexpr auto partial_sum_impl(std::index_sequence<Is...>, Args&& ...args) {
    return sum(get<Is>(std::forward<Args>(args)...)...);
}

template<std::size_t N, typename ...Args>
constexpr auto partial_sum(Args&& ...args) {
    return partial_sum_impl<N>(std::make_index_sequence<N>(), std::forward<Args>(args)...);
}

// multi_call

template<typename F_type, F_type F, typename ...Ts, std::size_t... Is, typename ...Args>
constexpr auto multi_call_impl(std::index_sequence<Is...>, Args&&... args) {
    return F(pp::partial_call<Ts, partial_sum<Is>(Ts::arity...)>(std::forward<Args>(args)...)...);
}

template<typename F_type, F_type F, typename ...Ts, typename ...Args>
constexpr auto multi_call(Args&&... args) {
    return multi_call_impl<F_type, F, Ts...>(std::index_sequence_for<Ts...>(), std::forward<Args>(args)...);
}

} // namespace

#endif
