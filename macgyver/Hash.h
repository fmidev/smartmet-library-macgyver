#pragma once
#include "LocalDateTime.h"
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace Fmi
{
const std::size_t bad_hash = 6178996271928UL;

void hash_combine(std::size_t& seed, std::size_t value);


// FIXME: make hash_value to return HashValue instead of std::size_t.
//        That would be ABI muutos and would require recompilation of many
//        packages
struct HashValue
{
  std::size_t value;
  HashValue(std::size_t value) : value(value) {}
  operator std::size_t() const { return value; }
  HashValue(const HashValue& other) = default;
  HashValue& operator = (const HashValue& other) = default;
};

std::size_t hash_value(const std::string& str);
std::size_t hash_value(const Date& date);
std::size_t hash_value(const TimeDuration& duration);
std::size_t hash_value(const DateTime& time);
std::size_t hash_value(const LocalDateTime& time);
std::size_t hash_value(const TimeZonePtr& zone);

std::size_t hash_value(bool value);
std::size_t hash_value(char value);
std::size_t hash_value(signed char value);
std::size_t hash_value(unsigned char value);
std::size_t hash_value(char16_t value);
std::size_t hash_value(char32_t value);
std::size_t hash_value(wchar_t value);
std::size_t hash_value(short value);
std::size_t hash_value(unsigned short value);
std::size_t hash_value(int value);
std::size_t hash_value(unsigned int value);
std::size_t hash_value(long value);
std::size_t hash_value(long long value);
std::size_t hash_value(unsigned long value);
std::size_t hash_value(unsigned long long value);
std::size_t hash_value(float value);
std::size_t hash_value(double value);
std::size_t hash_value(long double value);
inline std::size_t hash_value(const HashValue& hv) { return hv.value; }

// Optional objects with a normal hash_value implementation
template <typename T>
inline std::size_t hash_value(const boost::optional<T>& obj)
{
  if (!obj)
    return std::hash<bool>{}(false);
  std::size_t hash = std::hash<bool>{}(true);
  hash_combine(hash, hash_value(*obj));
  return hash;
}

template <typename T>
inline std::size_t hash_value(const std::optional<T>& obj)
{
  if (!obj)
    return std::hash<bool>{}(false);
  std::size_t hash = std::hash<bool>{}(true);
  hash_combine(hash, hash_value(*obj));
  return hash;
}

// std::shared_ptr
template <typename T>
inline std::size_t hash_value(const std::shared_ptr<T>& obj)
{
  if (!obj)
    return std::hash<bool>{}(false);
  std::size_t hash = std::hash<bool>{}(true);
  hash_combine(hash, hash_value(*obj));
  return hash;
}

// boost::shared ptr
template <typename T>
inline std::size_t hash_value(const boost::shared_ptr<T>& obj)
{
  if (!obj)
    return std::hash<bool>{}(false);
  std::size_t hash = std::hash<bool>{}(true);
  hash_combine(hash, hash_value(*obj));
  return hash;
}

// Maps
template <typename T, typename S>
inline std::size_t hash_value(const std::map<T, S>& objs)
{
  std::size_t hash = 333333333333;  // empty map value
  for (const auto& obj : objs)
  {
    hash_combine(hash, hash_value(obj.first));
    hash_combine(hash, hash_value(obj.second));
  }
  return hash;
}

// Vectors of objects
template <typename T>
inline std::size_t hash_value(const std::vector<T>& objs)
{
  std::size_t hash = 5555555555;  // empt vector value
  for (const auto& obj : objs)
    hash_combine(hash, hash_value(obj));
  return hash;
}

// Lists of objects
template <typename T>
inline std::size_t hash_value(const std::list<T>& objs)
{
  std::size_t hash = 7777777777;  // empty list value
  for (const auto& obj : objs)
    hash_combine(hash, hash_value(obj));
  return hash;
}

// Sets of objects
template <typename T>
inline std::size_t hash_value(const std::set<T>& objs)
{
  std::size_t hash = 9999999999;  // empty set value
  for (const auto& obj : objs)
    hash_combine(hash, hash_value(obj));
  return hash;
}

template <typename T, typename... Ts>
std::size_t hash(const T& obj1, Ts &&... ts)
{
  std::size_t val = hash_value(obj1);
  ([&]{ hash_combine(val, hash_value(ts)); }(), ...);
  return val;
}

template <typename... Ts>
void hash_merge(std::size_t& hash, Ts &&... ts)
{
  ([&]{ hash_combine(hash, hash_value(ts)); }(), ...);
}

// Objects in smartmet* packages may have methods for calculating hash values with names
// HashValue, hashValue or hash_value. Implement Fmi::hash_value for these. Handle correctly
// cases when more than one of these methods which all return std::size_t is available
// (avoid ambiguity).
template <typename T>
typename std::enable_if<std::is_same<std::size_t, decltype(std::declval<T>().HashValue())>::value, std::size_t>::type
hash_value(const T& obj)
{
  return obj.HashValue();
}

template <typename T>
typename std::enable_if<std::is_same<std::size_t, decltype(std::declval<T>().hashValue())>::value
                        && !std::is_same<std::size_t, decltype(std::declval<T>().HashValue())>::value,
         std::size_t>::type
hash_value(const T& obj)
{
  return obj.hashValue();
}

template <typename T>
typename std::enable_if<std::is_same<std::size_t, decltype(std::declval<T>().hash_value())>::value
                        && !std::is_same<std::size_t, decltype(std::declval<T>().HashValue())>::value
                        && !std::is_same<std::size_t, decltype(std::declval<T>().hashValue())>::value,
           std::size_t>::type
hash_value(const T& obj)
{
  return obj.hash_value();
}

}  // namespace Fmi
