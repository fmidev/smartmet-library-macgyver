#include "AsyncTaskGroup.h"
#include "TypeName.h"

Fmi::AsyncTaskGroup::AsyncTaskGroup(std::size_t max_paralell_tasks)
    : counter(0)
    , max_paralell_tasks(max_paralell_tasks)
    , num_suceeded(0)
    , num_failed(0)
    , stop_requested(false)
{
}

Fmi::AsyncTaskGroup::~AsyncTaskGroup()
{
    stop();
    wait();
}

void Fmi::AsyncTaskGroup::add(const std::string& name, std::function<void()> task)
{
  while (get_num_active_tasks() >= max_paralell_tasks) {
    wait_some();
  }

  std::unique_lock<std::mutex> lock(m1);
  if (!stop_requested) { 
    std::size_t id = ++counter;
    std::shared_ptr<AsyncTask> new_task(new AsyncTask(name, task,
            std::bind(&Fmi::AsyncTaskGroup::on_task_completed_callback, this, id)));
    active_tasks[id] = new_task;
  }
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
  stop_requested = true;
  for (auto item : active_tasks) {
      item.second->cancel();
  }
}

std::size_t Fmi::AsyncTaskGroup::get_num_active_tasks() const
{
  std::unique_lock<std::mutex> lock(m1);
  return active_tasks.size();
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

bool Fmi::AsyncTaskGroup::stop_on_error(bool enable)
{
  return stop_on_error_.exchange(enable);
}

bool Fmi::AsyncTaskGroup::wait_some()
{
  std::mutex m2;
  std::unique_lock<std::mutex> lock(m2);
  cond.wait(lock, [this] () {
      std::unique_lock<std::mutex> lock(m1);
      return active_tasks.empty() || !completed_tasks.empty();
  });
  lock.unlock();

  return handle_finished();
}

bool Fmi::AsyncTaskGroup::handle_finished()
{
  std::unique_lock<std::mutex> lock2(m1);
  if (completed_tasks.empty()) {
      return false;
  } else  {
      bool some_failed = false;
      std::shared_ptr<AsyncTask> task = completed_tasks.front();
      completed_tasks.pop();
      lock2.unlock();
      try {
          task->wait();
          // Notify through signal only if done not interrupted
          if (task->get_status() == AsyncTask::ok) {
              signal_task_ended(task->get_name());
              num_suceeded++;
          }
      } catch (...) {
          some_failed = true;
          num_failed++;
          signal_task_failed(task->get_name());
      }

      if (stop_on_error_ && some_failed) {
          stop();
          throw std::runtime_error(METHOD_NAME + " : one ore more tasks failed with C++ exceptions."
              " Stopping remaining tasks");
      }

      return true;
  }
}

void Fmi::AsyncTaskGroup::on_task_completed_callback(std::size_t task_id)
{
    std::unique_lock<std::mutex> lock(m1);
    auto it = active_tasks.find(task_id);
    if (it == active_tasks.end()) {
        // Should never happen
        throw std::runtime_error(" [INTERNAL ERROR] " + METHOD_NAME + " : task " + std::to_string(task_id) + " is not found");
    } else {
        completed_tasks.push(it->second);
        active_tasks.erase(it);
        lock.unlock();
        cond.notify_all();
    }
}
