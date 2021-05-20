#include "Hash.h"
#include "StringConversion.h"
#include <limits>

namespace
{
template <typename T>
T xorshift(const T& n, int i)
{
  return n ^ (n >> i);
}

uint32_t distribute(const uint32_t& n)
{
  uint32_t p = 0x55555555ul;  // pattern of alternating 0 and 1
  uint32_t c = 3423571495ul;  // random uneven integer constant;
  return c * xorshift(p * xorshift(n, 16), 16);
}

std::size_t hash(std::size_t n)
{
  std::size_t p = 0x5555555555555555;       // pattern of alternating 0 and 1
  std::size_t c = 17316035218449499591ull;  // random uneven integer constant;
  return c * xorshift(p * xorshift(n, 32), 32);
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

std::size_t hash_value(const boost::posix_time::ptime& time)
{
  return hash_value(Fmi::to_iso_string(time));
}

std::size_t hash_value(const boost::local_time::local_date_time& time)
{
  auto hash = Fmi::hash_value(time.local_time());
  Fmi::hash_combine(hash, Fmi::hash_value(time.zone()));
  return hash;
}

std::size_t hash_value(const boost::local_time::time_zone_ptr& zone)
{
  return Fmi::hash_value(zone->std_zone_name());
}

std::size_t hash_value(bool value)
{
  return hash(static_cast<std::size_t>(value));
}

std::size_t hash_value(char value)
{
  return hash(static_cast<std::size_t>(value));
}

std::size_t hash_value(signed char value)
{
  return hash(static_cast<std::size_t>(value));
}

std::size_t hash_value(unsigned char value)
{
  return hash(static_cast<std::size_t>(value));
}

std::size_t hash_value(char16_t value)
{
  return hash(static_cast<std::size_t>(value));
}

std::size_t hash_value(char32_t value)
{
  return hash(static_cast<std::size_t>(value));
}

std::size_t hash_value(wchar_t value)
{
  return hash(static_cast<std::size_t>(value));
}

std::size_t hash_value(short value)
{
  return hash(static_cast<std::size_t>(value));
}

std::size_t hash_value(unsigned short value)
{
  return hash(static_cast<std::size_t>(value));
}

std::size_t hash_value(int value)
{
  return hash(static_cast<std::size_t>(value));
}

std::size_t hash_value(unsigned int value)
{
  return hash(static_cast<std::size_t>(value));
}

std::size_t hash_value(long value)
{
  return hash(static_cast<std::size_t>(value));
}

std::size_t hash_value(long long value)
{
  return hash(static_cast<std::size_t>(value));
}

std::size_t hash_value(unsigned long value)
{
  return hash(static_cast<std::size_t>(value));
}

std::size_t hash_value(unsigned long long value)
{
  return hash(static_cast<std::size_t>(value));
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
    seed = rotl(seed, std::numeric_limits<size_t>::digits / 3) ^ distribute(hash(value));
}

}  // namespace Fmi
