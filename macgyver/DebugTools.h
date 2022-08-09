#pragma once

#include "TypeName.h"
#include <exception>
#include <iostream>
#include <string>

namespace Fmi
{
/**
 *   Class that performing logging of its object life time
 *
 *   To use create local object of this class in scope which
 *   execution time must be measured, like:
 *
 *   @code
 *   void foo()
 *   {
 *      Fmi::ScopedTimer timer(__FUNCTION__);
 *      // do something ...
 *      ...
 *   }
 *   @endcode
 */
class ScopedTimer
{
 public:
  ScopedTimer(const std::string& theName);
  virtual ~ScopedTimer();

 private:
  const std::string name;
  std::string time_str;
  double start;
};

/**
 *  @brief Returns tracer PID under Linux (or always 0 for Windows)
 */
int tracerPid();

/**
 *   @brief Debugging macro for showing exceptions thrown by a function
 *
 *   Possible use:
 *   boost test framwork do not report exceptions type and related info in case of use
 *   of macros line BOOST_CHECK_NO_THROW(). On can use this macro to workaround this
 *   problem:
 *   @code
 *   BOOST_CHECK_NO_THROW(foo = SHOW_EXCEPTIONS(bar());
 *   @endcode
 */
#define SHOW_EXCEPTIONS(x)                                                                        \
  [&]()                                                                                           \
  {                                                                                               \
    try                                                                                           \
    {                                                                                             \
      return x;                                                                                   \
    }                                                                                             \
    catch (const std::exception& e)                                                               \
    {                                                                                             \
      std::cout << "Exception " << Fmi::current_exception_type() << ": " << e.what() << std::endl \
                << "    thrown by '" << #x << '\'' << std::endl                                   \
                << "    in " << __FILE__ << " at line " << __LINE__ << std::endl;                 \
      throw;                                                                                      \
    }                                                                                             \
  }()

}  // namespace Fmi
