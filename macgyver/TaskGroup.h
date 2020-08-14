#pragma once

// FIXME: remove after replaced by AsyncTaskGroup in WFS plugin

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <boost/signals2.hpp>

namespace Fmi {

class TaskGroup
{
  struct Task;
 public:
  TaskGroup(std::size_t max_parallel_tasks = 30);
  virtual ~TaskGroup();

  operator bool () const { return get_num_failures() != 0; }

  void add(const std::string& name, std::function<void()> task);
  void wait();
  std::size_t get_num_tasks() const;
  std::size_t get_num_failures() const;

  boost::signals2::connection on_task_ended(std::function<void(const std::string&)> callback);
  boost::signals2::connection on_task_error(std::function<void(const std::string&)> callback);

 private:
  bool wait_some();
  bool extract_finished(std::queue<std::shared_ptr<Task> >& finished_tasks);
  void notify();

 private:
  typedef std::list<std::shared_ptr<Task> >::iterator iterator;

  mutable std::mutex mutex;
  std::condition_variable cond;
  std::size_t max_parallel_tasks;
  std::size_t counter;
  std::atomic<std::size_t> num_failures;
  std::list<std::shared_ptr<Task> > task_list;
  boost::signals2::signal<void(const std::string&)> signal_task_ended;
  boost::signals2::signal<void(const std::string&)> signal_task_failed;
};

}
