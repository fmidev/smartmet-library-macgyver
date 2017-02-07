// ======================================================================
/*!
 * \brief Directory change monitor
 */
// ======================================================================

#pragma once

#include <boost/filesystem.hpp>
#include <boost/function.hpp>
#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <ctime>
#include <map>
#include <memory>
#include <string>

namespace Fmi
{
class DirectoryMonitor : private boost::noncopyable
{
 public:
  // Watcher ID

  typedef std::size_t Watcher;

  // Change type

  typedef int Change;

  // Observable events

  static const Change NONE = 0x00;    // no changes
  static const Change CREATE = 0x01;  // new file created (or first pass)
  static const Change DELETE = 0x02;  // old file deleted
  static const Change MODIFY = 0x04;  // old file modified
  static const Change ERROR = 0x08;   // error occured
  static const Change ALL = 0x0f;     // any change

  // Directory listing with modification state

  typedef std::map<boost::filesystem::path, Change> StatusMap;
  typedef boost::shared_ptr<StatusMap> Status;

  DirectoryMonitor();
  ~DirectoryMonitor();

  // Callback interfaces. Note that const references could be used
  // since a write lock exists during the callback. However, if
  // the callback instantiates a new thread and passes the
  // referenced variables on to the new thread, new copies should
  // be made. Since we cannot enforce such code in this API,
  // overall it is safer just to make new copies directly
  // in the callback. Callbacks do not occur that often, so the
  // overhead is reasonably small compared to possible damage.

  // Can we actually enforce the API here? What if the called API
  // actually declares references instead of this API? A simple
  // test shows that the code compiles if the callee declares
  // references instead of copies.

  typedef boost::function<void(
      Watcher id, boost::filesystem::path path, boost::regex pattern, Status status)> Listener;

  typedef boost::function<void(
      Watcher id, boost::filesystem::path path, boost::regex pattern, std::string message)>
      ErrorHandler;

  // Request new monitored path/regex

  Watcher watch(const boost::filesystem::path& path,
                const boost::regex& pattern,
                Listener callback,
                ErrorHandler errorhandler,
                int interval = 60,
                Change mask = ALL);

  // Request new monitored path, without regex
  Watcher watch(const boost::filesystem::path& path,
                Listener callback,
                ErrorHandler errorhandler,
                int interval = 60,
                Change mask = ALL);

  // Start monitoring
  void run();

  // Stop monitoring
  void stop();

  // Return true if at least once scan has been completed
  bool ready() const;

 private:
  DirectoryMonitor(const DirectoryMonitor& other);
  DirectoryMonitor& operator=(const DirectoryMonitor& other);

  class Pimple;
  std::unique_ptr<Pimple> impl;

};  // class DirectoryMonitor
}  // namespace Fmi

// ======================================================================
