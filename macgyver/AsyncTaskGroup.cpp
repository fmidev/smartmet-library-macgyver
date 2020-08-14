#include "AsyncTaskGroup.h"

Fmi::AsyncTaskGroup::AsyncTaskGroup(std::size_t max_paralell_tasks)
    : max_paralell_tasks(max_paralell_tasks)
    , total_tasks(0)
    , num_suceeded(0)
    , num_failed(0)
{
}

Fmi::AsyncTaskGroup::~AsyncTaskGroup()
{
}

void Fmi::AsyncTaskGroup::add(const std::string& name, std::function<void()> task)
{
  while (get_num_active_tasks() >= max_paralell_tasks) {
    wait_some();
  }

  total_tasks++;
  std::unique_lock<std::mutex> lock(m1);
  task_list.emplace_back(new AsyncTask(name, task, &cond));
}

void Fmi::AsyncTaskGroup::wait()
{
    while (wait_some())
    {
    }
}

void Fmi::AsyncTaskGroup::stop()
{
  std::unique_lock<std::mutex> lock(m1);
  for (std::shared_ptr<AsyncTask> task : task_list) {
      task->cancel();
  }
}

std::size_t Fmi::AsyncTaskGroup::get_num_active_tasks() const
{
  std::unique_lock<std::mutex> lock(m1);
  return task_list.size();
}

boost::signals2::connection
Fmi::AsyncTaskGroup::on_task_ended(std::function<void(const std::string&)> callback)
{
  return signal_task_ended.connect(callback);
}

boost::signals2::connection
Fmi::AsyncTaskGroup::on_task_error(std::function<void(const std::string&)> callback)
{
  return signal_task_failed.connect(callback);
}

bool Fmi::AsyncTaskGroup::wait_some()
{
  std::queue<std::shared_ptr<Fmi::AsyncTask> > finished_tasks;
  std::unique_lock<std::mutex> lock(m1);
  cond.wait(lock, [this, &finished_tasks] () { return extract_finished(finished_tasks); });
  lock.unlock();

  if (finished_tasks.empty()) {
      return false;
  } else {
      std::shared_ptr<AsyncTask> task = finished_tasks.front();
      finished_tasks.pop();
      try {
          task->wait();
          // Notify through signal only if done not interrupted
          if (task->get_status() == AsyncTask::ok) {
              signal_task_ended(task->get_name());
              num_suceeded++;
          }
      } catch (...) {
          num_failed++;
          signal_task_failed(task->get_name());
      }
      return true;
  }
}

bool Fmi::AsyncTaskGroup::extract_finished(std::queue<std::shared_ptr<Fmi::AsyncTask> >& finished_tasks)
{
  // Must be called with mutex m1 locked
  typedef decltype(task_list)::iterator iterator;
  bool empty = task_list.empty();
  bool found = false;
  iterator curr, next;
  for (curr = task_list.begin(); curr != task_list.end(); curr = next) {
    next = curr;
    ++next;
    if ((*curr)->ended()) {
      found = true;
      finished_tasks.push(*curr);
      task_list.erase(curr);
    }
  }
  return empty || found;
}
