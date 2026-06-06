#include <functional>

namespace Fmi
{
/**
 * @brief StaticCleanup class provides a way to register cleanup functions
 *
 * StaticCleanup class provides a way to register cleanup functions that are
 * called when the last instance of AtExit class is destroyed.
 *
 * The cleanup functions are called in the reverse order they were registered.
 *
 * It is intended for use of global/static objects that need to be cleaned up
 * while the program is still in a well-defined state, i.e. before unordered
 * static destruction at program exit. The typical use is breaking the
 * static-destruction-order fiasco between a cache of objects backed by some
 * third party global state (e.g. GDAL/PROJ) and that library's own statics.
 *
 * Usage contract (important):
 *   - Register cleanups by constructing a StaticCleanup (usually as a
 *     function-local static next to the object it cleans up).
 *   - Construct exactly one AtExit as a *local* variable at the top of main()
 *     (or a test driver). When it is destroyed as main() returns, the
 *     cleanups run while everything is still alive.
 *   - AtExit instances may be nested; cleanups run when the outermost one is
 *     destroyed. They run at most once: the registered functions are cleared
 *     after being invoked, so a later AtExit cycle will not re-run them.
 *   - Do NOT give an AtExit instance static/global lifetime. Its lifetime
 *     must nest inside main() so that it is destroyed before the statics that
 *     the cleanup functions refer to.
 *
 * Implementation note: the registry (the list of cleanup functions, the
 * instance counter and the guarding mutex) is intentionally never destroyed
 * (it is a leaked process-lifetime singleton). This avoids relying on the order
 * of static destruction and, importantly, avoids a double-free on toolchains
 * that run a namespace-scope object's destructor twice at exit (observed with
 * GCC on some platforms: once via __cxa_finalize and once via the DSO's
 * __do_global_dtors_aux). The leak is bounded, one-off and reachable at exit.
 */
class StaticCleanup final
{
 public:
  StaticCleanup(std::function<void()> cleanup_function);
  ~StaticCleanup();

  // Each instance registers exactly one cleanup function; copying or moving
  // would not register anything but could still be misused, so disallow it.
  StaticCleanup(const StaticCleanup&) = delete;
  StaticCleanup(StaticCleanup&&) = delete;
  StaticCleanup& operator=(const StaticCleanup&) = delete;
  StaticCleanup& operator=(StaticCleanup&&) = delete;

  class AtExit final
  {
   public:
    AtExit();
    ~AtExit();

    // The instance count is balanced by ctor/dtor pairs. A copy or move would
    // run an extra destructor without a matching constructor, corrupting the
    // count and triggering (or suppressing) cleanup at the wrong time.
    AtExit(const AtExit&) = delete;
    AtExit(AtExit&&) = delete;
    AtExit& operator=(const AtExit&) = delete;
    AtExit& operator=(AtExit&&) = delete;
  };
};
}  // namespace Fmi
