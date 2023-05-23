#pragma once

#include <type_traits>
#include <boost/type_traits/make_void.hpp>

namespace Fmi
{

//=============================================================================================
// Detecting whether type is iterable
//
// Source: https://en.cppreference.com/w/cpp/types/void_t
//
// Variable template that checks if a type has begin() and end() member functions
// std::void_t is replaced as it is available begining from C++17
//

template< typename... Ts >
using void_t = typename boost::make_void<Ts...>::type;

template <typename T, typename = void>
struct is_iterable : std::false_type {};

// this gets used only when we can call std::begin() and std::end() on that type
template <typename T>
struct is_iterable<T, void_t<decltype(std::begin(std::declval<T&>())),
                             decltype(std::end(std::declval<T&>())) >
                  > : std::true_type {};

//=============================================================================================

}
