#include "DebugTools.h"
#include "StringConversion.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <array>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#ifndef _MSC_VER
#include <sys/time.h>
#endif

Fmi::ScopedTimer::ScopedTimer(const std::string& theName) : name(theName)
{
#ifndef _MSC_VER
  std::array<char, 80> buffer{};
  struct tm t2;
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  start = tv.tv_sec + 0.000001 * tv.tv_usec;
  static_cast<void>(localtime_r(&tv.tv_sec, &t2));  // would just return &t2
  static_cast<void>(strftime(buffer.data(), buffer.size(), "%Y-%m-%d %H:%M:%S", &t2));
  time_str = buffer.data();
  static_cast<void>(snprintf(buffer.data(), buffer.size(), ".%06u", static_cast<unsigned>(tv.tv_usec)));
  time_str += buffer.data();
#else
// VC++ toteutus
#endif
}

Fmi::ScopedTimer::~ScopedTimer()
{
#ifndef _MSC_VER
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  double end = tv.tv_sec + 0.000001 * tv.tv_usec;
  double dt = end - start;

  std::ostringstream msg;
  msg << time_str << ": " << name << ": " << dt << " seconds\n";
  std::cout << msg.str() << std::flush;
#else
  std::cout << "Fmi::ScopedTimer not implemented with Visual C++";
#endif
}

Fmi::Redirecter::Redirecter(std::ostream& dest, std::ostream& src) : src(src)
{
  src << std::flush;
  sbuf = src.rdbuf(dest.rdbuf());
}

Fmi::Redirecter::~Redirecter()
{
  src << std::flush;
  src.rdbuf(sbuf);
}

int Fmi::tracerPid()
{
#ifndef _MSC_VER
  using namespace boost::algorithm;
  std::string line;
  std::ifstream input("/proc/self/status");
  while (std::getline(input, line))
  {
    std::vector<std::string> parts;
    split(parts, line, is_any_of(" \t\r\n"), token_compress_on);
    if (parts.size() == 2 && parts[0] == "TracerPid:")
    {
      return Fmi::stoi(parts[1]);
    }
  }
#endif
  return 0;
}
