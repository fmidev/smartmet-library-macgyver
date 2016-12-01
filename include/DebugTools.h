#pragma once

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
}
