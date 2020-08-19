#include "AsyncTask.h"
#include <chrono>
#include <iostream>
#include <boost/chrono.hpp>
#include "TypeName.h"

bool Fmi::AsyncTask::silent = false;

Fmi::AsyncTask::AsyncTask(const std::string& name, std::function<void()> task, std::condition_variable* cond)
    : name(name)
    , status(none)
    , done(false)
    , cond(cond)
    , ex(nullptr)
    , task_thread([this, task] () { run(task); })
{
}

Fmi::AsyncTask::~AsyncTask()
{
    if (task_thread.joinable()) {
        try {
            wait();
        } catch (const std::exception& e) {
            if (!silent) {
                std::cerr << '[' << METHOD_NAME << "] WARNING: Ignoring exception of type '"
                          << Fmi::current_exception_type() << "' from async task '" << name << "': "
                          << e.what() << std::endl;
            }
        } catch (...) {
            if (!silent) {
                std::cerr << '[' << METHOD_NAME << "] WARNING: Ignoring exception of type '"
                          << Fmi::current_exception_type() << "' from async task '" << name
                          << std::endl;
            }
        }
    }
}

void Fmi::AsyncTask::wait()
{
    if (task_thread.joinable()) {
        task_thread.join();
        std::exception_ptr ex = get_exception();
        if (ex) {
            std::rethrow_exception(ex);
        }
    }
}

bool Fmi::AsyncTask::wait_for(double sec)
{
    if (task_thread.joinable()) {
        int64_t mks = int64_t(std::ceil(1000000.0 * sec));
        bool done = task_thread.try_join_for(boost::chrono::microseconds(mks));
        if (done) {
            std::exception_ptr ex = get_exception();
            if (ex) {
                std::rethrow_exception(ex);
            }
        }
        return done;
    } else {
        return false;
    }
}

void Fmi::AsyncTask::cancel()
{
    std::unique_lock<std::mutex> lock(m1);
    if (!done) {
        task_thread.interrupt();
    }
}

Fmi::AsyncTask::Status Fmi::AsyncTask::get_status() const
{
    return status;
}

void Fmi::AsyncTask::interruption_point()
{
    boost::this_thread::interruption_point();
}

void Fmi::AsyncTask::run(std::function<void()> task)
{
    try
    {
        status = active;
        task();
        handle_result(ok, nullptr);
    } catch (boost::thread_interrupted&) {
        handle_result(interrupted, nullptr);
    } catch (...) {
        handle_result(failed, std::current_exception());
    }
}

void Fmi::AsyncTask::handle_result(Status status, std::exception_ptr ex)
{
    std::unique_lock<std::mutex> lock(m1);
    this->done = true;
    this->status = status;
    this->ex = ex;
    lock.unlock();
    if (cond) {
        (*cond).notify_all();
    }
}

std::exception_ptr Fmi::AsyncTask::get_exception() const
{
    std::unique_lock<std::mutex> lock(m1);
    return ex;
}
