#include "DebugTools.h"
#include <ctime>
#include <cstdio>
#include <iostream>
#include <sstream>

#ifndef _MSC_VER
#include <sys/time.h>
#endif

Fmi::ScopedTimer::ScopedTimer(const std::string& theName) : name(theName)
{
#ifndef _MSC_VER
  char buffer[80];
  struct tm t2;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  start = tv.tv_sec + 0.000001 * tv.tv_usec;
  localtime_r(&tv.tv_sec, &t2);
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &t2);
  time_str = buffer;
  snprintf(buffer, sizeof(buffer), ".%06u", static_cast<unsigned>(tv.tv_usec));
  time_str += buffer;
#else
// VC++ toteutus
#endif
}

Fmi::ScopedTimer::~ScopedTimer()
{
#ifndef _MSC_VER
  struct timeval tv;
  gettimeofday(&tv, NULL);
  double end = tv.tv_sec + 0.000001 * tv.tv_usec;
  double dt = end - start;

  std::ostringstream msg;
  msg << time_str << ": " << name << ": " << dt << " seconds" << std::endl;
  std::cout << msg.str() << std::flush;
#else
  std::cout << "Fmi::ScopedTimer not implemented with Visual C++" << std::flush;
#endif
}
