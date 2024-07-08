//
// Adds std::optional stream I/O missing in libstdc++
//
// Taken with small modifications from boost (boost/optional/optional_io.hpp).
//
#pragma once

#include <istream>
#include <optional>
#include <ostream>

namespace Fmi
{

template<class CharType, class CharTrait>
inline
std::basic_ostream<CharType, CharTrait>&
operator<<(std::basic_ostream<CharType, CharTrait>& out, std::nullopt_t)
{
  if (out.good())
  {
    out << "--";
  }

  return out;
}

template<class CharType, class CharTrait, class T>
inline
std::basic_ostream<CharType, CharTrait>&
operator<<(std::basic_ostream<CharType, CharTrait>& out, const std::optional<T> const& v)
{
  if (out.good())
  {
    if (!v)
         out << "--" ;
    else out << ' ' << *v ;
  }

  return out;
}

template<class CharType, class CharTrait, class T>
inline
std::basic_istream<CharType, CharTrait>&
operator>>(std::basic_istream<CharType, CharTrait>& in, std::optional<T>& v)
{
  if (in.good())
  {
    int d = in.get();
    if (d == ' ')
    {
      T x;
      in >> x;
      v = x;
    }
    else
    {
      if (d == '-')
      {
        d = in.get();

        if (d == '-')
        {
          v = std::nullopt;
          return in;
        }
      }

      in.setstate( std::ios::failbit );
    }
  }

  return in;
}

}
