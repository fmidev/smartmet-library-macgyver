#pragma once
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace Fmi
{
const std::size_t bad_hash = 6178996271928UL;

void hash_combine(std::size_t& seed, std::size_t value);

template <typename T>
std::size_t hash_value(const T& value)
{
  return std::hash<T>{}(value);
}

std::size_t hash_value(const std::string& str);
std::size_t hash_value(const boost::posix_time::ptime& time);
std::size_t hash_value(const boost::local_time::local_date_time& time);
std::size_t hash_value(const boost::local_time::time_zone_ptr& zone);

// Optional objects with a normal hash_value implementation
template <typename T>
inline std::size_t hash_value(const boost::optional<T>& obj)
{
  if (!obj)
    return std::hash<bool>{}(false);
  else
  {
    std::size_t hash = std::hash<bool>{}(true);
    hash_combine(hash, hash_value(*obj));
    return hash;
  }
}

// std::shared_ptr
template <typename T>
inline std::size_t hash_value(const std::shared_ptr<T>& obj)
{
  if (!obj)
    return std::hash<bool>{}(false);
  else
  {
    std::size_t hash = std::hash<bool>{}(true);
    hash_combine(hash, hash_value(*obj));
    return hash;
  }
}

// boost::shared ptr
template <typename T>
inline std::size_t hash_value(const boost::shared_ptr<T>& obj)
{
  if (!obj)
    return std::hash<bool>{}(false);
  else
  {
    std::size_t hash = std::hash<bool>{}(true);
    hash_combine(hash, hash_value(*obj));
    return hash;
  }
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

}  // namespace Fmi
