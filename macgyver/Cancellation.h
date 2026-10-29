// ======================================================================
/*!
 * \brief Cooperative cancellation of the work done for one request
 *
 * The server creates a token for each request and makes it current in the thread
 * which calls the plugin. Code which may run for long calls Fmi::check_cancelled()
 * in its loops. The call throws once the request has been cancelled or its deadline
 * has passed, and does nothing in a thread without a current token, for example in
 * the tests of a plugin or in a background task.
 *
 * Do not check in code which computes something shared with other requests, such as
 * a value which other requests wait for in a cache. They would fail with the
 * cancelled request.
 *
 * Work moved to another thread is cancelled only if the token is made current there
 * too:
 *
 *   auto token = Fmi::current_cancellation_token();
 *   ... in the other thread:
 *   Fmi::CancellationScope scope(token);
 */
// ======================================================================

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <limits>
#include <memory>
#include <optional>

namespace Fmi
{
class Exception;

class CancellationToken
{
 public:
  using Clock = std::chrono::steady_clock;

  enum class Reason
  {
    None,          // the request may continue
    Deadline,      // the time limit of the request was exceeded
    Disconnected,  // the client closed the connection
    Cancelled      // cancelled for some other reason
  };

  // Polls the state of the request, for example whether the client is still
  // connected. Returns Reason::None while the request may continue.
  using Probe = std::function<Reason()>;

  CancellationToken() = default;
  ~CancellationToken() = default;
  CancellationToken(const CancellationToken& other) = delete;
  CancellationToken(CancellationToken&& other) = delete;
  CancellationToken& operator=(const CancellationToken& other) = delete;
  CancellationToken& operator=(CancellationToken&& other) = delete;

  // Cancel the request. The first reason is kept.
  void cancel(Reason theReason = Reason::Cancelled) noexcept;

  // The time by which the request must be complete
  void setDeadline(Clock::time_point theDeadline) noexcept;
  std::optional<Clock::time_point> deadline() const noexcept;

  // Poll the probe at most once per interval. Must be set before the token is used
  // by other threads.
  void setProbe(Probe theProbe, Clock::duration theInterval);

  // Reason::None while the request may continue. Notices an expired deadline and
  // polls the probe when it is time to.
  Reason reason() const noexcept;
  bool cancelled() const noexcept { return reason() != Reason::None; }

  // Throws an exception recognized by is_cancellation() if the request has been
  // cancelled
  void check() const;

  // Whether check() has thrown, that is, whether the processing of the request was
  // cut short and its response is not to be trusted
  bool interrupted() const noexcept { return itsInterrupted.load(); }

 private:
  using Rep = Clock::rep;
  void setReason(Reason theReason) const noexcept;

  static constexpr Rep no_deadline = std::numeric_limits<Rep>::max();

  mutable std::atomic<Reason> itsReason{Reason::None};
  mutable std::atomic<bool> itsInterrupted{false};
  std::atomic<Rep> itsDeadline{no_deadline};

  Probe itsProbe;
  Clock::duration itsProbeInterval{};
  mutable std::atomic<Rep> itsNextProbe{0};
};

// Description of the reason, for example for messages to the client
const char* to_string(CancellationToken::Reason theReason);

// Whether the exception or one of the exceptions it wraps was thrown because the
// request was cancelled
bool is_cancellation(const Exception& theException);

// The token current in this thread, empty if none
std::shared_ptr<CancellationToken> current_cancellation_token();

// Throws if the token current in this thread has been cancelled, does nothing if
// there is no current token
void check_cancelled();

// Makes a token current in this thread until destroyed, the previous token is then
// restored
class CancellationScope
{
 public:
  explicit CancellationScope(std::shared_ptr<CancellationToken> theToken);
  ~CancellationScope();
  CancellationScope(const CancellationScope& other) = delete;
  CancellationScope(CancellationScope&& other) = delete;
  CancellationScope& operator=(const CancellationScope& other) = delete;
  CancellationScope& operator=(CancellationScope&& other) = delete;

 private:
  std::shared_ptr<CancellationToken> itsPrevious;
};

}  // namespace Fmi
