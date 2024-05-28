#include "Hash.h"
#include "StringConversion.h"
#include <limits>

namespace
{
std::size_t xorshift(std::size_t n, int i)
{
  return n ^ (n >> i);
}

std::size_t hash_int(std::size_t n)
{
  std::size_t p = 0x5555555555555555;                // pattern of alternating 0 and 1
  std::size_t c = 17316035218449499591ull;           // random uneven integer constant;
  return c * xorshift(p * xorshift(n + p, 32), 32);  // added p to n to get nonzero result for zero
}

std::size_t rotl(std::size_t n, std::size_t i)
{
  const std::size_t m = (std::numeric_limits<std::size_t>::digits - 1);
  const std::size_t c = i & m;
  return (n << c) | (n >> ((std::size_t(0) - c) & m));
}

}  // namespace

namespace Fmi
{
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

std::size_t hash_value(bool value)
{
  return hash_int(static_cast<std::size_t>(value));
}

std::size_t hash_value(char value)
{
  return hash_int(static_cast<std::size_t>(value));
}

std::size_t hash_value(signed char value)
{
  return hash_int(static_cast<std::size_t>(value));
}

std::size_t hash_value(unsigned char value)
{
  return hash_int(static_cast<std::size_t>(value));
}

std::size_t hash_value(char16_t value)
{
  return hash_int(static_cast<std::size_t>(value));
}

std::size_t hash_value(char32_t value)
{
  return hash_int(static_cast<std::size_t>(value));
}

std::size_t hash_value(wchar_t value)
{
  return hash_int(static_cast<std::size_t>(value));
}

std::size_t hash_value(short value)
{
  return hash_int(static_cast<std::size_t>(value));
}

std::size_t hash_value(unsigned short value)
{
  return hash_int(static_cast<std::size_t>(value));
}

std::size_t hash_value(int value)
{
  return hash_int(static_cast<std::size_t>(value));
}

std::size_t hash_value(unsigned int value)
{
  return hash_int(static_cast<std::size_t>(value));
}

std::size_t hash_value(long value)
{
  return hash_int(static_cast<std::size_t>(value));
}

std::size_t hash_value(long long value)
{
  return hash_int(static_cast<std::size_t>(value));
}

std::size_t hash_value(unsigned long value)
{
  return hash_int(static_cast<std::size_t>(value));
}

std::size_t hash_value(unsigned long long value)
{
  return hash_int(static_cast<std::size_t>(value));
}

std::size_t hash_value(float value)
{
  return std::hash<float>{}(value);
}

std::size_t hash_value(double value)
{
  return std::hash<double>{}(value);
}

std::size_t hash_value(long double value)
{
  return std::hash<long double>{}(value);
}

/*
 * Boost hash_combine produces collisions too often.
 *
 * Ref:
 * https://stackoverflow.com/questions/35985960/c-why-is-boosthash-combine-the-best-way-to-combine-hash-values/50978188#50978188
 */

void hash_combine(std::size_t& seed, std::size_t value)
{
  if (seed == bad_hash || value == bad_hash)
    seed = bad_hash;
  else
    seed = rotl(seed, std::numeric_limits<size_t>::digits / 3) ^ hash_int(value);
}

}  // namespace Fmi
