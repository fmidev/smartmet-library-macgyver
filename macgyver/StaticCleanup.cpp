#include "StaticCleanup.h"
#include <list>
#include <mutex>

namespace Fmi
{
namespace
{
// The registry is intentionally never destroyed (leaked process-lifetime
// singletons). See the note in StaticCleanup.h: this avoids relying on static
// destruction order and a double-free on toolchains that run a namespace-scope
// object's destructor twice at exit. The leak is bounded and reachable at exit.

std::mutex& mutex()
{
  static std::mutex* instance = new std::mutex;
  return *instance;
}

std::list<std::function<void()>>& cleanup_functions()
{
  static auto* instance = new std::list<std::function<void()>>;
  return *instance;
}

unsigned& instance_count()
{
  static unsigned* instance = new unsigned(0);
  return *instance;
}
}  // namespace

StaticCleanup::StaticCleanup(std::function<void()> cleanup_function)
{
  std::lock_guard<std::mutex> lock(mutex());
  cleanup_functions().push_front(std::move(cleanup_function));
}

StaticCleanup::~StaticCleanup() = default;

StaticCleanup::AtExit::AtExit()
{
  std::lock_guard<std::mutex> lock(mutex());
  ++instance_count();
}

StaticCleanup::AtExit::~AtExit()
{
  // Move the cleanup functions out under the lock when the last AtExit is
  // destroyed, then run them without holding the lock. Moving them out means
  // they run at most once even across multiple AtExit create/destroy cycles,
  // and releasing the lock first avoids deadlock should a cleanup register a
  // new StaticCleanup.
  std::list<std::function<void()>> functions;
  {
    std::lock_guard<std::mutex> lock(mutex());
    if (--instance_count() != 0)
      return;
    functions.swap(cleanup_functions());
  }

  for (const auto& cleanup_function : functions)
  {
    cleanup_function();
  }
}
}  // namespace Fmi
