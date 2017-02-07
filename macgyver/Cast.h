// ======================================================================
/*!
 * \brief Conversion tools
 */
// ======================================================================

#ifndef FMI_CAST_H
#define FMI_CAST_H

#include <boost/lexical_cast.hpp>
#include <limits>
#include <stdexcept>
#include <string>

#if defined(_MSC_VER) && (_MSC_VER < 1600)  // c++11 tukea ja esim. int64_t ei ole MSVC++
                                            // kääntäjissä ennen VC++ 2010 eli _MSC_VER < 1600,
                                            // joten se tuki on totettava boostista
#include <boost/cstdint.hpp>
typedef boost::int64_t int64_t;
typedef boost::uint64_t uint64_t;
#endif

namespace Fmi
{
// ----------------------------------------------------------------------
/*!
 * \brief Type information
 */
// ----------------------------------------------------------------------

template <typename T>
inline const char* number_name()
{
  return "number";
}

template <>
inline const char* number_name<char>()
{
  return "char";
}
template <>
inline const char* number_name<int>()
{
  return "int";
}
template <>
inline const char* number_name<short>()
{
  return "short";
}
template <>
inline const char* number_name<long>()
{
  return "long";
}

template <>
inline const char* number_name<unsigned char>()
{
  return "unsigned char";
}
template <>
inline const char* number_name<unsigned int>()
{
  return "unsigned int";
}
template <>
inline const char* number_name<unsigned short>()
{
  return "unsigned short";
}
template <>
inline const char* number_name<unsigned long>()
{
  return "unsigned long";
}

template <>
inline const char* number_name<float>()
{
  return "float";
}
template <>
inline const char* number_name<double>()
{
  return "double";
}

// ----------------------------------------------------------------------
/*!
 * \brief Lexical cast with informative exceptions
 */
// ----------------------------------------------------------------------

template <typename T>
T number_cast(const std::string& theValue)
{
  try
  {
    return boost::lexical_cast<T>(theValue);
  }
  catch (boost::bad_lexical_cast&)
  {
    throw std::runtime_error("number_cast failed to convert '" + theValue + "' to a " +
                             number_name<T>());
  }
}

template <typename T>
T number_cast(const char* theValue)
{
  try
  {
    return boost::lexical_cast<T>(theValue);
  }
  catch (boost::bad_lexical_cast&)
  {
    throw std::runtime_error(std::string("number_cast failed to convert '") + theValue + "' to a " +
                             number_name<T>());
  }
}

/**
 *   @brief Alternative conversion to integer with additional range check
 */
int64_t str2int(const std::string& text,
                int64_t lower_limit = std::numeric_limits<int64_t>::min(),
                int64_t upper_limit = std::numeric_limits<int64_t>::max());

/**
 *   @brief Alternative conversion to unsigned integer with additional range check
 */
uint64_t str2uint(const std::string& text,
                  uint64_t lower_limit = std::numeric_limits<uint64_t>::min(),
                  uint64_t upper_limit = std::numeric_limits<uint64_t>::max());

/**
 *   @brief Alternative conversion to double with additional range check
 */
double str2double(const std::string& text,
                  double lower_limit = -std::numeric_limits<double>::max(),
                  double upper_limit = std::numeric_limits<double>::max());

}  // namespace Fmi

#endif  // FMI_CAST_H

// ======================================================================
