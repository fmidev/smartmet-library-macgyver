#include "Cancellation.h"
#include "Exception.h"
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
  itsReason.compare_exchange_strong(expected, theReason);
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
  Reason reason = itsReason.load(std::memory_order_relaxed);
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
