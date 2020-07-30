#include "TaskGroup.h"

#ifdef _MSC_VER
// This enables 'not', 'or', 'and', etc logical keywords in Visual C++ compilers (implemented by
// macro definitions),// use !, ||, &&, etc instead, if you want to get rid of this ugly inclusion.
#include <ciso646>
#endif

Fmi::TaskGroup::TaskGroup(std::size_t max_parallel_tasks)
  : max_parallel_tasks(max_parallel_tasks)
  , counter(0)
  , num_failures(0)
{
}

Fmi::TaskGroup::~TaskGroup()
{
}

void Fmi::TaskGroup::add(const std::string& name, std::function<void()> task)
{
  while (get_num_tasks() >= max_parallel_tasks) {
    wait_some();
  }

  std::function<void()> task_impl = [this, task]()
    {
      try {
	task();
      } catch (...) {
	notify();
	throw;
      }

      notify();
    };

  std::unique_lock<std::mutex> lock(mutex);
  task_list.push_back(std::make_pair(name, std::async(std::launch::async, task_impl).share()));
}

void Fmi::TaskGroup::wait()
{
  while (wait_some()) {
  }
}

std::size_t Fmi::TaskGroup::get_num_tasks() const
{
  std::lock_guard<std::mutex> lock(mutex);
  return task_list.size();
}

std::size_t Fmi::TaskGroup::get_num_failures() const
{
  return num_failures;
}

boost::signals2::connection
Fmi::TaskGroup::on_task_ended(std::function<void(const std::string&)> callback)
{
  return signal_task_ended.connect(callback);
}

boost::signals2::connection
Fmi::TaskGroup::on_task_error(std::function<void(const std::string&)> callback)
{
  return signal_task_failed.connect(callback);
}

bool Fmi::TaskGroup::wait_some()
{
  std::queue<std::pair<std::string, std::shared_future<void> > > finished_tasks;
  std::unique_lock<std::mutex> lock(mutex);
  if (task_list.empty()) {
    return false;
  } else {
    extract_finished(finished_tasks);
    if (finished_tasks.empty()) {
      cond.wait(lock);
      extract_finished(finished_tasks);
    }
  }

  lock.unlock();

  while (not finished_tasks.empty()) {
    auto item = finished_tasks.front();
    finished_tasks.pop();
    try {
      item.second.get();
      signal_task_ended(item.first);
    } catch (...) {
      num_failures++;
      signal_task_failed(item.first);
    }
  }

  return true;
}

void
Fmi::TaskGroup::extract_finished(std::queue<std::pair<std::string, std::shared_future<void> > >& finished_tasks)
{
  iterator curr, next;
  for (curr = task_list.begin(); curr != task_list.end(); curr = next) {
    next = curr;
    ++next;
    if (curr->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
      finished_tasks.push(*curr);
      task_list.erase(curr);
    }
  }
}

void Fmi::TaskGroup::notify()
{
  std::unique_lock<std::mutex> lock(mutex);
  cond.notify_all();
};

