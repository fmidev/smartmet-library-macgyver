#pragma once

#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>

#include <atomic>
#include <condition_variable>
#include <mutex>

namespace Fmi
{
class AsyncTask : private virtual boost::noncopyable
{
 public:
  enum Status
  {
    none,
    active,
    ok,
    failed,
    interrupted
  };

  typedef void notify_finished_t(const std::string&, Status);

  static bool silent;

 public:
  /**
   *   @brief Constructor: creates AsyncTask object
   *
   *   @param name Name of task for logging purpose
   *   @param task function to perform in the task
   *   @param notify function to call when requested function has ended (any reason - success,
   * interrupted, exception thrown)
   *
   *   Notification callback is called from task thread, so locking may be required
   */
  AsyncTask(const std::string& name,
            std::function<void()> task,
            std::function<void()> notify = std::function<void()>());

  virtual ~AsyncTask();

  void wait();

  /**
   *   @brief Stops the asynchronous task at interruption point
   */
  void cancel();

  bool wait_for(double sec);

  Status get_status() const;

  bool ended() const { return done; }

  const std::string& get_name() const { return name; }

  static void interruption_point();

 private:
  void run(std::function<void()> task);
  void handle_result(Status stat, std::exception_ptr exc = nullptr);
  std::exception_ptr get_exception() const;

 private:
  const std::string name;
  mutable std::mutex m1;
  std::atomic<Status> status;
  std::atomic<bool> done;
  const std::function<void()> notify;
  std::exception_ptr ex;
  boost::thread task_thread;
};
}  // namespace Fmi
