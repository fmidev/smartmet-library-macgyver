#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>

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
    AsyncTask(const std::string& name, std::function<void()> task, std::condition_variable* cond = nullptr);
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
    void handle_result(Status status, std::exception_ptr exc = nullptr);

  private:
    const std::string name;
    std::mutex m1;
    std::atomic<Status> status;
    std::atomic<bool> done;
    std::atomic<std::condition_variable*> cond;
    boost::thread task_thread;
    std::exception_ptr ex;
  };
}
