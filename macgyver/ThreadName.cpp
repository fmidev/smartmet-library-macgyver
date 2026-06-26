#include "ThreadName.h"

#include <fmt/format.h>
#include <iostream>
#include <mutex>
#include <set>

#if defined(__linux__)
#include <pthread.h>
#endif

namespace Fmi
{
void set_thread_name(const std::string& name)
{
#if defined(__linux__)
  // Linux thread names are limited to 16 bytes including the terminating NUL,
  // i.e. 15 visible characters. Warn once per distinct name if it is too long
  // so that the truncation does not silently hide thread identities.
  if (name.size() > 15)
  {
    static std::mutex warned_mutex;
    static std::set<std::string> warned;
    const std::lock_guard<std::mutex> lock(warned_mutex);
    if (warned.insert(name).second)
    {
      std::cerr << fmt::format(
          "WARNING: thread name '{}' exceeds the 15 character limit and is truncated to '{}'\n",
          name,
          name.substr(0, 15));
    }
  }
  pthread_setname_np(pthread_self(), name.substr(0, 15).c_str());
#else
  (void)name;
#endif
}

}  // namespace Fmi
