#pragma once

#include <atomic>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <boost/noncopyable.hpp>
#include <boost/signals2.hpp>
#include "AsyncTask.h"

namespace Fmi
{
  class AsyncTaskGroup : public boost::noncopyable
  {
  public:
    AsyncTaskGroup(std::size_t max_paralell_tasks = 30);
    virtual ~AsyncTaskGroup();

    void add(const std::string& name, std::function<void()> task);
    void wait();
    void stop();

    /**
     *  @brief Total number of tasks added
     */
    std::size_t get_task_count() const { return total_tasks; }

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
     *  Called from method wait()
     */
    boost::signals2::connection on_task_error(std::function<void(const std::string&)> callback);

  private:
    bool wait_some();
    bool extract_finished(std::queue<std::shared_ptr<Fmi::AsyncTask> >& finished_tasks);

  private:
    std::size_t max_paralell_tasks;
    mutable std::mutex m1;
    std::condition_variable cond;
    std::list<std::shared_ptr<AsyncTask> > task_list;
    std::atomic<std::size_t> total_tasks;
    std::atomic<std::size_t> num_suceeded;
    std::atomic<std::size_t> num_failed;
    boost::signals2::signal<void(const std::string&)> signal_task_ended;
    boost::signals2::signal<void(const std::string&)> signal_task_failed;
  };
};