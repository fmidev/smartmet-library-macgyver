#include "TaskGroup.h"

struct Fmi::TaskGroup::Task
{
  std::string name;
  std::atomic<bool> done;
  std::atomic<bool> failed;
  Fmi::TaskGroup* tg;
  std::shared_future<void> f;

  Task(const std::string& name, std::function<void()> task, Fmi::TaskGroup* tg)
    : name(name)
    , done(false)
    , failed(false)
    , tg(tg)
    , f(std::async(std::launch::async, [this, task]() { execute(task); }).share())
    {
    }

private:
    void execute(std::function<void()> task)
    {
      try {
        task();
        done = true;
        tg->notify();
      } catch (...) {
        done = true;
        failed = true;
        tg->notify();
        throw;
      }
    }
};

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

  std::unique_lock<std::mutex> lock(mutex);
  task_list.emplace_back(new Task(name, task, this));
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
  std::queue<std::shared_ptr<Task> > finished_tasks;
  std::unique_lock<std::mutex> lock(mutex);
  cond.wait(lock, [this, &finished_tasks]() { return extract_finished(finished_tasks); });
  lock.unlock();

  if (finished_tasks.empty()) {
    return false;
  } else {
    while (not finished_tasks.empty()) {
      auto item = finished_tasks.front();
      finished_tasks.pop();
      try {
        item->f.get();
        signal_task_ended(item->name);
      } catch (...) {
      num_failures++;
      signal_task_failed(item->name);
      }
    }
    return true;
  }
}

bool
Fmi::TaskGroup::extract_finished(std::queue<std::shared_ptr<Fmi::TaskGroup::Task> >& finished_tasks)
{
  bool empty = task_list.empty();
  bool found = false;
  iterator curr, next;
  for (curr = task_list.begin(); curr != task_list.end(); curr = next) {
    next = curr;
    ++next;
    if ((*curr)->done) {
      found = true;
      finished_tasks.push(*curr);
      task_list.erase(curr);
    }
  }
  return empty || found;
}

void Fmi::TaskGroup::notify()
{
  std::unique_lock<std::mutex> lock(mutex);
  cond.notify_all();
};

