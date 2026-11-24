#include "Cancellation.h"
#include "Exception.h"
#include <algorithm>
#include <cstring>

namespace Fmi
{
namespace
{
// The what() of the exception thrown by CancellationToken::check()
const char* const cancellation_error = "RequestCancelled";

thread_local std::shared_ptr<CancellationToken> current_token;
}  // namespace

void CancellationToken::setReason(Reason theReason) const noexcept
{
  Reason expected = Reason::None;
  if (itsReason.compare_exchange_strong(expected, theReason))
    runCallbacks();
}

// Called once, by the thread which cancelled the token
void CancellationToken::runCallbacks() const noexcept
{
  std::unique_lock<std::mutex> lock(itsCallbackMutex);
  itsCallbacksCalled = true;
  itsCallbackThread = std::this_thread::get_id();
  while (!itsCallbacks.empty())
  {
    const auto* callback = itsCallbacks.back();
    itsCallbacks.pop_back();
    itsRunningCallback = callback;
    lock.unlock();
    callback->call();
    lock.lock();
    itsRunningCallback = nullptr;
    itsCallbackDone.notify_all();
  }
}

void CancellationToken::beginWatch() noexcept
{
  ++itsWatchers;
}

void CancellationToken::endWatch() noexcept
{
  --itsWatchers;
}

void CancellationToken::cancel(Reason theReason) noexcept
{
  if (theReason != Reason::None)
    setReason(theReason);
}

void CancellationToken::setDeadline(Clock::time_point theDeadline) noexcept
{
  itsDeadline = theDeadline.time_since_epoch().count();
}

std::optional<CancellationToken::Clock::time_point> CancellationToken::deadline() const noexcept
{
  const Rep deadline = itsDeadline;
  if (deadline == no_deadline)
    return std::nullopt;
  return Clock::time_point(Clock::duration(deadline));
}

void CancellationToken::setProbe(Probe theProbe, Clock::duration theInterval)
{
  itsProbe = std::move(theProbe);
  itsProbeInterval = theInterval;
}

CancellationToken::Reason CancellationToken::reason() const noexcept
{
  const Reason reason = itsReason.load(std::memory_order_relaxed);
  if (reason != Reason::None || watched())
    return reason;
  return poll();
}

CancellationToken::Reason CancellationToken::poll() const noexcept
{
  const Reason reason = itsReason.load(std::memory_order_relaxed);
  if (reason != Reason::None)
    return reason;

  const Rep now = Clock::now().time_since_epoch().count();

  if (now >= itsDeadline.load(std::memory_order_relaxed))
    setReason(Reason::Deadline);
  else if (itsProbe)
  {
    // Only one of the threads sharing the token polls when it is time to
    Rep next = itsNextProbe.load(std::memory_order_relaxed);
    if (now >= next &&
        itsNextProbe.compare_exchange_strong(next, now + itsProbeInterval.count()))
    {
      try
      {
        setReason(itsProbe());
      }
      catch (...)
      {
        // A failing probe does not cancel the request
      }
    }
  }

  return itsReason.load();
}

void CancellationToken::check() const
{
  const Reason reason = this->reason();
  if (reason == Reason::None)
    return;

  itsInterrupted = true;
  throw Fmi::Exception(BCP, cancellation_error)
      .addParameter("description", to_string(reason))
      .disableLogging();
}

const char* to_string(CancellationToken::Reason theReason)
{
  switch (theReason)
  {
    case CancellationToken::Reason::None:
      return "Not cancelled";
    case CancellationToken::Reason::Deadline:
      return "The request was not completed within its time limit";
    case CancellationToken::Reason::Disconnected:
      return "The client closed the connection";
    case CancellationToken::Reason::Cancelled:
      break;
  }
  return "The request was cancelled";
}

bool is_cancellation(const Exception& theException)
{
  for (const auto* e = &theException; e != nullptr; e = e->getPrevException())
  {
    const char* what = e->getWhat();
    if (what != nullptr && std::strcmp(what, cancellation_error) == 0)
      return true;
  }
  return false;
}

std::shared_ptr<CancellationToken> current_cancellation_token()
{
  return current_token;
}

void check_cancelled()
{
  if (current_token)
    current_token->check();
}

CancellationCallback::CancellationCallback(std::function<void()> theFunction)
    : CancellationCallback(current_token, std::move(theFunction))
{
}

CancellationCallback::CancellationCallback(std::shared_ptr<CancellationToken> theToken,
                                           std::function<void()> theFunction)
    : itsToken(std::move(theToken)), itsFunction(std::move(theFunction))
{
  if (!itsToken || !itsFunction)
    return;

  {
    std::lock_guard<std::mutex> lock(itsToken->itsCallbackMutex);
    if (!itsToken->itsCallbacksCalled)
    {
      itsToken->itsCallbacks.push_back(this);
      return;
    }
  }

  // Already cancelled
  call();
}

CancellationCallback::~CancellationCallback()
{
  if (!itsToken || !itsFunction)
    return;

  std::unique_lock<std::mutex> lock(itsToken->itsCallbackMutex);
  auto& callbacks = itsToken->itsCallbacks;
  const auto pos = std::find(callbacks.begin(), callbacks.end(), this);
  if (pos != callbacks.end())
  {
    callbacks.erase(pos);
    return;
  }

  // Wait if another thread is calling the function, a call from this thread is in progress
  // only if the function destroys its own callback
  if (itsToken->itsCallbackThread != std::this_thread::get_id())
    itsToken->itsCallbackDone.wait(lock, [this]() { return itsToken->itsRunningCallback != this; });
}

void CancellationCallback::call() const noexcept
{
  try
  {
    itsFunction();
  }
  catch (...)
  {
    // A failure to break off the call does not change the cancellation
  }
}

CancellationScope::CancellationScope(std::shared_ptr<CancellationToken> theToken)
    : itsPrevious(std::move(current_token))
{
  current_token = std::move(theToken);
}

CancellationScope::~CancellationScope()
{
  current_token = std::move(itsPrevious);
}

}  // namespace Fmi
