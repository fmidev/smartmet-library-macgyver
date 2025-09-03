#include "Hash.h"
#include "StringConversion.h"
#include <limits>

namespace
{

// Based on Boost 1.81 code, our current production Boost is too old.

// MurmurHash finalization mix
std::size_t hash_mix(std::size_t n)
{
  n ^= n >> 33;
  n *= 0xff51afd7ed558ccd;
  n ^= n >> 33;
  n *= 0xc4ceb9fe1a85ec53;
  n ^= n >> 33;
  return n;
}

}  // namespace

namespace Fmi
{
std::size_t hash_value(bool value)
{
  return hash_mix(static_cast<std::size_t>(value));
}

std::size_t hash_value(char value)
{
  return hash_mix(static_cast<std::size_t>(value));
}

std::size_t hash_value(signed char value)
{
  return hash_mix(static_cast<std::size_t>(value));
}

std::size_t hash_value(unsigned char value)
{
  return hash_mix(static_cast<std::size_t>(value));
}

std::size_t hash_value(char16_t value)
{
  return hash_mix(static_cast<std::size_t>(value));
}

std::size_t hash_value(char32_t value)
{
  return hash_mix(static_cast<std::size_t>(value));
}

std::size_t hash_value(wchar_t value)
{
  return hash_mix(static_cast<std::size_t>(value));
}

std::size_t hash_value(short value)
{
  return hash_mix(static_cast<std::size_t>(value));
}

std::size_t hash_value(unsigned short value)
{
  return hash_mix(static_cast<std::size_t>(value));
}

std::size_t hash_value(int value)
{
  return hash_mix(static_cast<std::size_t>(value));
}

std::size_t hash_value(unsigned int value)
{
  return hash_mix(static_cast<std::size_t>(value));
}

std::size_t hash_value(long value)
{
  return hash_mix(static_cast<std::size_t>(value));
}

std::size_t hash_value(long long value)
{
  return hash_mix(static_cast<std::size_t>(value));
}

std::size_t hash_value(unsigned long value)
{
  return hash_mix(static_cast<std::size_t>(value));
}

std::size_t hash_value(unsigned long long value)
{
  return hash_mix(static_cast<std::size_t>(value));
}

std::size_t hash_value(float value)
{
  return (value == 0.0f) ? 0 : hash_mix(static_cast<std::size_t>(*(unsigned int*)(&value)));
}

std::size_t hash_value(double value)
{
  return (value == 0.0) ? 0 : hash_mix(static_cast<std::size_t>(*(unsigned long long*)(&value)));
}

std::size_t hash_value(long double value)
{
  return (value == 0.0L) ? 0 : hash_mix(static_cast<std::size_t>(*(unsigned long long*)(&value)));
}

// This should probably be replaced by siphash for better collision protection
std::size_t hash_value(const std::string& str)
{
  return std::hash<std::string>{}(str);
}

std::size_t hash_value(const Fmi::Date& date)
{
  const auto type = date.type();
  auto hash = hash_value(1024 + static_cast<int>(type));
  if (type == Fmi::date_time::Base::NORMAL)
    hash_combine(hash, hash_value(date.get_impl().time_since_epoch().count()));
  return hash;
}

std::size_t hash_value(const TimeDuration& duration)
{
  const auto type = duration.type();
  auto hash = hash_value(2048 + static_cast<int>(type));
  if (type == Fmi::date_time::Base::NORMAL)
    hash_combine(hash, hash_value(duration.get_impl().count()));
  return hash;
}

std::size_t hash_value(const DateTime& time)
{
  const auto type = time.type();
  auto hash = hash_value(3072 + static_cast<int>(type));
  if (type == Fmi::date_time::Base::NORMAL)
    hash_combine(hash, hash_value(time.get_impl().time_since_epoch().count()));
  return hash;
}

std::size_t hash_value(const Fmi::LocalDateTime& time)
{
  // Getting UTC time gets local time at first and then converts it to UTC
  // As result local time is used here for better efficiency
  auto hash = Fmi::hash_value(time.local_time());
  Fmi::hash_combine(hash, Fmi::hash_value(time.zone()));
  return hash;
}

std::size_t hash_value(const Fmi::TimeZonePtr& zone)
{
  return Fmi::hash_value(zone->name());
}

// Boost 1.81 algorithm

void hash_combine(std::size_t& seed, std::size_t value)
{
  if (seed == bad_hash || value == bad_hash)
    seed = bad_hash;
  else
    seed = hash_mix(seed + 0x9e3779b9 + value);
}

}  // namespace Fmi
