// ======================================================================
/*!
 * \brief Implementation of class DirectoryMonitor
 *
 * The implementation depends heavily on the Schedule object,
 * which essentially consists of a list of monitors sorted
 * by their scheduled update times.
 *
 * If the is no pending check, the run-loop sleeps until the
 * first scheduled update. Waking up, the run-loop checks
 * the scheduled monitors until it runs into a monitor
 * whose scheduled update time is in the future. The time sort
 * is updated after each monitor update, so all the run-loop
 * has to do is keep checking the first scheduled monitor
 * to decide what to do.
 *
 * Upon an observed change, the run-loop calls the listeners
 * sequentially. The listeners may start a new thread to
 * perform any update necessary, the decision is up to the
 * user.
 *
 * The DirectoryMonitor is intended to be used like this:
 * \code
 * Fmi::DirectoryMonitor mon;
 * mon.watch(...);
 * ...
 * boost::thread thrd(boost::bind(&DirectoryMonitor::run,&mon));
 * ...
 * // Explicit stop:
 * mon.stop();
 * // Timed stop:
 * thrd.timed_join(boost::posix_time::seconds(N));
 * \endcode
 * The behaviour is unspecified if the destructor is called while  typedef
 boost::interprocess::interprocess_upgradable_mutex MutexType;
  typedef boost::interprocess::upgradable_lock<MutexType> WriteLock;
  typedef boost::interprocess::sharable_lock<MutexType> ReadLock;

 * the thread is still running.
 *
 * It is possible to run the monitor single threaded, but in that
 * case it is possible to stop the monitor only by calling the stop
 * method from the callback function (or by signals).
 *
 */
// ======================================================================

#include "DirectoryMonitor.h"
#include "StringConversion.h"

#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

//#ifdef FMI_MULTITHREAD
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
//#endif

#include <stdexcept>

// scoped read/write lock types

#ifdef FMI_MULTITHREAD
typedef boost::shared_mutex MutexType;
typedef boost::shared_lock<MutexType> ReadLock;
typedef boost::unique_lock<MutexType> WriteLock;
#else
struct MutexType
{
};
struct ReadLock
{
  ReadLock(const MutexType& /* mutex */) {}
};
struct WriteLock
{
  WriteLock(const MutexType& /* mutex */) {}
};
#endif

namespace fs = boost::filesystem;

namespace Fmi
{
// ----------------------------------------------------------------------
/*!
 * \brief Directory content with modification times
 */
// ----------------------------------------------------------------------

typedef std::map<boost::filesystem::path, std::time_t> Contents;

// ----------------------------------------------------------------------
/*!
 * \brief Scan directory contents
 */
// ----------------------------------------------------------------------

Contents directory_contents(const fs::path& path, bool hasregex, const boost::regex& pattern)
{
  Contents contents;

  // safety against missing dir.
  // in case of single files, the file has been removed

  if (!fs::exists(path)) return contents;

  // find all matching filenames with modification times
  // if target is a directory

  if (fs::is_directory(path))
  {
    fs::directory_iterator end;
    for (fs::directory_iterator it(path); it != end; ++it)
    {
      // Check first that the filename does not start with '.'
      char dot = '.';
      if (it->path().filename().native().at(0) != dot)
      {
        if (hasregex)
        {
          if (boost::regex_match(it->path().filename().string(), pattern))
          {
            std::time_t t = fs::last_write_time(it->path());
            contents.insert(Contents::value_type(it->path(), t));
          }
        }

        else
        {
          std::time_t t = fs::last_write_time(it->path());
          contents.insert(Contents::value_type(it->path(), t));
        }
      }
    }
  }
  else
  {
    // No regex checking for single files
    std::time_t t = fs::last_write_time(path);
    contents.insert(Contents::value_type(path, t));
  }
  return contents;
}

// ----------------------------------------------------------------------
/*!
 * \brief Scan for changes in a directory
 *
 * Changes:
 * - create = new file created
 * - delete = old file deleted
 * - modify = old file modified
 */
// ----------------------------------------------------------------------

std::pair<DirectoryMonitor::Status, DirectoryMonitor::Change> directory_change(
    const Contents& oldcontents, const Contents& newcontents)
{
  DirectoryMonitor::Status status(new DirectoryMonitor::StatusMap);

  DirectoryMonitor::Change changes = DirectoryMonitor::NONE;

  // Scan old contents detecting modifications and deletes

  BOOST_FOREACH (const Contents::value_type& it, oldcontents)
  {
    Contents::const_iterator pos = newcontents.find(it.first);

    DirectoryMonitor::Change change = DirectoryMonitor::NONE;

    if (pos == newcontents.end())
      change = DirectoryMonitor::DELETE;
    else if (it.second != pos->second)
      change = DirectoryMonitor::MODIFY;

    changes |= change;
    (*status)[it.first] = change;
  }

  // Scan new contents detecting new files

  BOOST_FOREACH (const Contents::value_type& it, newcontents)
  {
    Contents::const_iterator pos = oldcontents.find(it.first);
    if (pos == oldcontents.end())
    {
      changes |= DirectoryMonitor::CREATE;
      (*status)[it.first] = DirectoryMonitor::CREATE;
    }
  }

  return std::make_pair(status, changes);
}

// ----------------------------------------------------------------------
/*
 * \brief Information on a single monitored path/regex
 */
// ----------------------------------------------------------------------

struct Monitor
{
  // static info:
  fs::path path;
  boost::regex pattern;
  bool hasregex;
  int interval;
  DirectoryMonitor::Watcher id;
  DirectoryMonitor::Change mask;
  DirectoryMonitor::Listener callback;
  DirectoryMonitor::ErrorHandler errorhandler;

  // generated data:
  time_t lastmodified;
  Contents contents;
};

// ----------------------------------------------------------------------
/*
 * \brief Information on all monitors
 *
 * The index is the time of the next scheduled update. If the time
 * is zero, it indicates no previous update has been done.
 */
// ----------------------------------------------------------------------

typedef std::multimap<std::time_t, Monitor> Schedule;

// ----------------------------------------------------------------------
/*!
 * \brief Implementation interface
 */
// ----------------------------------------------------------------------

class DirectoryMonitor::Pimple
{
 public:
  MutexType mutex;
  Schedule schedule;
  bool running;  // true if run() has not exited
  bool stop;     // true if stop request is pending
  bool isready;  // true if at least one scan has completed
  Watcher nextid;

  Pimple() : mutex(), schedule(), running(false), stop(false), isready(false), nextid(0) {}
};

// ----------------------------------------------------------------------
/*!
 * \brief Constructor
 */
// ----------------------------------------------------------------------

DirectoryMonitor::DirectoryMonitor() : impl(new Pimple()) {}
// ----------------------------------------------------------------------
/*!
 * \brief Destructor
 */
// ----------------------------------------------------------------------

DirectoryMonitor::~DirectoryMonitor() {}
// ----------------------------------------------------------------------
/*
 * \brief Request a new monitored path
 */
// ----------------------------------------------------------------------

DirectoryMonitor::Watcher DirectoryMonitor::watch(const fs::path& path,
                                                  const boost::regex& pattern,
                                                  Listener callback,
                                                  ErrorHandler errorhandler,
                                                  int interval,
                                                  Change mask)
{
  if (interval < 1)
    throw std::runtime_error("DirectoryMonitor: Too small update interval: " +
                             Fmi::to_string(interval));

  if ((mask & ALL) == 0)
    throw std::runtime_error("DirectoryMonitor: Empty mask, nothing to monitor");

  // if(!fs::exists(path))
  //   throw std::runtime_error("DirectoryMonitor: "+path.string()+" does not exist");

  // new monitor

  WriteLock lock(impl->mutex);

  Monitor mon;
  mon.path = path;
  mon.pattern = pattern;
  mon.interval = interval;
  mon.mask = mask;
  mon.callback = callback;
  mon.errorhandler = errorhandler;
  mon.id = impl->nextid;
  mon.lastmodified = 0;
  mon.hasregex = true;

  ++impl->nextid;

  // time_t = 0 implies no update has been made yet by run()

  impl->schedule.insert(std::make_pair(0, mon));

  return mon.id;
}

// ----------------------------------------------------------------------
/*
 * \brief Request a new monitored path without regex
 */
// ----------------------------------------------------------------------

DirectoryMonitor::Watcher DirectoryMonitor::watch(
    const fs::path& path, Listener callback, ErrorHandler errorhandler, int interval, Change mask)
{
  if (interval < 1)
    throw std::runtime_error("DirectoryMonitor: Too small update interval: " +
                             Fmi::to_string(interval));

  if ((mask & ALL) == 0)
    throw std::runtime_error("DirectoryMonitor: Empty mask, nothing to monitor");

  // if(!fs::exists(path))
  //   throw std::runtime_error("DirectoryMonitor: "+path.string()+" does not exist");

  // new monitor

  WriteLock lock(impl->mutex);

  Monitor mon;
  mon.path = path;
  mon.interval = interval;
  mon.mask = mask;
  mon.callback = callback;
  mon.errorhandler = errorhandler;
  mon.id = impl->nextid;
  mon.lastmodified = 0;
  mon.hasregex = false;

  ++impl->nextid;

  // time_t = 0 implies no update has been made yet by run()

  impl->schedule.insert(std::make_pair(0, mon));

  return mon.id;
}

// ----------------------------------------------------------------------
/*
 * \brief Start monitoring
 */
// ----------------------------------------------------------------------

void DirectoryMonitor::run()
{
  // Do not start if already running

  // Quick exit without locking if possible
  if (impl->running) return;

  {
    WriteLock lock(impl->mutex);
    if (impl->running) return;
    impl->running = true;
  }

  while (!impl->stop && !impl->schedule.empty())
  {
    bool checknext = true;

    while (checknext)
    {
      WriteLock lock(impl->mutex);
      std::time_t tnow = std::time(NULL);
      std::time_t tcheck = impl->schedule.begin()->first;

      if (tcheck > tnow)
        checknext = false;
      else
      {
        // pop the monitor from the schedule, insert it back
        // later on with updated information

        Monitor mon = impl->schedule.begin()->second;
        impl->schedule.erase(impl->schedule.begin());

        // first check dir modification time

        // establish nature of changes

        Change changes;
        Status newstatus(new DirectoryMonitor::StatusMap);

        try
        {
          std::time_t tchange;
          if (fs::exists(mon.path))
          {
            tchange = fs::last_write_time(mon.path);
          }
          else
          {
            tchange = std::time(NULL);
          }

          // We cannot detect modifications simply by looking
          // at the directory change time. In such cases we
          // must scan the directory contents.

          if (tchange <= mon.lastmodified && !((mon.mask & MODIFY) != 0))
          {
            // nothing to scan since dir timestamp did not change

            std::time_t tnext = std::time(NULL) + mon.interval;
            impl->schedule.insert(std::make_pair(tnext, mon));
          }
          else
          {
            // new listing must be taken since dir timestamp changed

            Contents newcontents = directory_contents(mon.path, mon.hasregex, mon.pattern);

            boost::tie(newstatus, changes) = directory_change(mon.contents, newcontents);

            // possible callback

            if ((changes & mon.mask) != 0)
            {
              mon.callback(mon.id, mon.path, mon.pattern, newstatus);
            }

            // update schedule and status

            mon.contents = newcontents;
            mon.lastmodified = tchange;

            std::time_t tnext = std::time(NULL) + mon.interval;
            impl->schedule.insert(std::make_pair(tnext, mon));
          }
        }
        catch (std::exception& e)
        {
#ifdef DEBUG
          std::cerr << "Warning: " << e.what() << std::endl;
#endif

          if ((mon.mask & ERROR) != 0)
          {
            mon.errorhandler(mon.id, mon.path, mon.pattern, e.what());
          }

          std::time_t tnext = std::time(NULL) + mon.interval;
          impl->schedule.insert(std::make_pair(tnext, mon));
        }
      }
    }

    // One scan has now been completed
    impl->isready = true;

    long sleeptime = 0;
    {
      ReadLock tmplock(impl->mutex);
      std::time_t tmpnow = std::time(NULL);
      std::time_t tmpcheck = impl->schedule.begin()->first;
      sleeptime = (tmpnow > tmpcheck ? 0 : tmpcheck - tmpnow);
    }

    if (sleeptime > 0)
    {
      //#ifdef FMI_MULTITHREAD
      boost::this_thread::sleep(boost::posix_time::seconds(sleeptime));
      //#else
      //			sleep(sleeptime);
      //#endif
    }
  }

  // Not running anymore. This order so that locking is not necessary

  impl->stop = false;
  impl->running = false;
}

// ----------------------------------------------------------------------
/*
 * \brief Stop monitoring
 *
 * This should be called from a thread other than the one which called
 * run(), since the exit condition variable in the loop in run() is
 * not changed inside the loop itself.
 */
// ----------------------------------------------------------------------

void DirectoryMonitor::stop()
{
  // locking not needed here
  impl->stop = true;
}

// ----------------------------------------------------------------------
/*!
 * \brief Check if at least once scan of all directories has been completed
 */
// ----------------------------------------------------------------------

bool DirectoryMonitor::ready() const { return impl->isready; }
}  // namespace Fmi

// ======================================================================
