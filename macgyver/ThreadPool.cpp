#include "ThreadPool.h"

namespace Fmi
{
namespace ThreadPool
{
namespace
{
thread_local boost::thread* current_thread = nullptr;
}

boost::thread* current_worker_thread()
{
  return current_thread;
}

void set_current_worker_thread(boost::thread* theThread)
{
  current_thread = theThread;
}

}  // namespace ThreadPool
}  // namespace Fmi
