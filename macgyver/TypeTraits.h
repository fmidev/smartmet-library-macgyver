#pragma once

#include <type_traits>

namespace Fmi
{

//=============================================================================================
// Detecting whether type is iterable
//
// Source: https://stackoverflow.com/questions/13830158/check-if-a-variable-type-is-iterable
//
template <typename T, typename = void>
struct is_iterable : std::false_type {};

// this gets used only when we can call std::begin() and std::end() on that type
template <typename T>
struct is_iterable
<
  T,
  std::void_t
  <
    decltype(std::begin(std::declval<T&>())),
    decltype(std::end(std::declval<T&>()))
  >
> : std::true_type {};

// Here is a helper:
template <typename T>
constexpr bool is_iterable_v = is_iterable<T>::value;

//=============================================================================================

}
