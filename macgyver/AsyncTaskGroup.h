#pragma once

#include "AsyncTask.h"

#include <boost/noncopyable.hpp>
#include <boost/signals2.hpp>

#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <queue>

namespace Fmi
{
class AsyncTaskGroup : public boost::noncopyable
{
 public:
  AsyncTaskGroup(std::size_t max_paralell_tasks = 30);
  virtual ~AsyncTaskGroup();

  /**
   *   @brief Add a new task
   *
   *   May block if the number of maximal paralell tasks is achieved
   *   (wait for some tasks ton complete)
   */
  void add(const std::string& name, std::function<void()> task);

  /**
   *   @brief Wait for all added tasks to end
   *
   *   Any type of ending is accepted:
   *   - success      - callback provided in call to method on_task_ended is called
   *   - failed       - unhandled C++ exception in task. Callback provided in call to
   *                    method on_task_failed is called from exception handler
   *   - interrupted  - task ended due to call to method stop
   */
  void wait();

  /**
   *   @brief Handle completion of already finished tasks (if there is any) and do not
   *          for any more to finish
   *
   *   Used callbacks are same as for method wait (wait uses handle_finished internally)
   */
  bool handle_finished();

  /**
   *   @brief Requests all active tasks to stop and do not accept any more
   */
  void stop();

  /**
   *  @brief Total number of tasks added
   */
  std::size_t get_task_count() const { return counter; }

  /**
   *  @brief Get number of tasks ended successfully
   */
  std::size_t get_tasks_succeeded() const { return num_suceeded; }

  /**
   *  @brief Get number of failed tasks
   *
   *  Only those which has thrown an exception. Canceled tasks are not included
   */
  std::size_t get_num_failures() const { return num_failed; }

  std::size_t get_num_active_tasks() const;

  /**
   *  @brief Specifies callback to be called when task has ended succesfully
   *
   *  Called from method wait()
   */
  boost::signals2::connection on_task_ended(std::function<void(const std::string&)> callback);

  /**
   *  @brief Specifies callback to be called when task has failed (threw as exception)
   *
   *  Called from method wait() while handling the exception.
   */
  boost::signals2::connection on_task_error(std::function<void(const std::string&)> callback);

  /**
   *  @brief Specifies whether all tasks should interrupted when one or more are noticed failing
   *         (exception thrown)
   */
  bool stop_on_error(bool enable);

 private:
  bool wait_some();
  void on_task_completed_callback(std::size_t task_id);

 private:
  std::atomic<std::size_t> counter;
  std::size_t max_paralell_tasks;
  mutable std::mutex m1;
  std::condition_variable cond;
  std::map<std::size_t, std::shared_ptr<AsyncTask> > active_tasks;
  std::queue<std::shared_ptr<AsyncTask> > completed_tasks;
  std::atomic<std::size_t> num_suceeded;
  std::atomic<std::size_t> num_failed;
  boost::signals2::signal<void(const std::string&)> signal_task_ended;
  boost::signals2::signal<void(const std::string&)> signal_task_failed;
  std::atomic<bool> stop_requested;
  std::atomic<bool> stop_on_error_;
};
}  // namespace Fmi
