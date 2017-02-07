#ifdef UNIX

#include "TypeName.h"
#include <cxxabi.h>
#include <cstdlib>

std::string Fmi::demangle_cpp_type_name(const std::string& src)
{
  int status;
  std::size_t length = 0;
  std::string result;
  char* name = abi::__cxa_demangle(src.c_str(), NULL, &length, &status);
  switch (status)
  {
    case 0:
      result = name;
      break;
    case -1:
      result = "<Failed>";
      break;
    default:
      result = "<Invalid>";
      break;
  }
  free(name);
  return result;
}

std::string Fmi::current_exception_type()
{
  return demangle_cpp_type_name(abi::__cxa_current_exception_type()->name());
}

#endif
