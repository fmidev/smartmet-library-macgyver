// ======================================================================
/*!
 * \file
 * \brief Regression tests for Fmi::CancellationToken
 */
// ======================================================================

#include "Cancellation.h"
#include "Exception.h"
#include <regression/tframe.h>
#include <atomic>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace CancellationTest
{
using Token = Fmi::CancellationToken;
using Reason = Fmi::CancellationToken::Reason;

// Returns the reason in the exception thrown by check(), None if nothing was thrown
std::string check_and_catch(const Token& token)
{
  try
  {
    token.check();
  }
  catch (const Fmi::Exception& e)
  {
    if (!Fmi::is_cancellation(e))
      return "not a cancellation: " + std::string(e.what());
    const char* description = e.getParameterValue("description");
    return description != nullptr ? description : "no description";
  }
  return "";
}

void not_cancelled()
{
  Token token;
  if (token.cancelled())
    TEST_FAILED("A new token must not be cancelled");
  if (token.deadline())
    TEST_FAILED("A new token must not have a deadline");
  if (!check_and_catch(token).empty())
    TEST_FAILED("check() must not throw for a new token");
  if (token.interrupted())
    TEST_FAILED("A token must not be interrupted before check() throws");
  TEST_PASSED();
}

void cancel()
{
  Token token;
  token.cancel(Reason::Disconnected);
  token.cancel(Reason::Deadline);
  if (token.reason() != Reason::Disconnected)
    TEST_FAILED("The first reason must be kept");
  if (token.interrupted())
    TEST_FAILED("Cancelling must not mark the token interrupted before check() throws");

  const auto result = check_and_catch(token);
  if (result != Fmi::to_string(Reason::Disconnected))
    TEST_FAILED("Unexpected exception: " + result);
  if (!token.interrupted())
    TEST_FAILED("The token must be interrupted after check() has thrown");
  TEST_PASSED();
}

void deadline()
{
  Token token;
  token.setDeadline(Token::Clock::now() + std::chrono::hours(1));
  if (token.cancelled())
    TEST_FAILED("A deadline in the future must not cancel");

  token.setDeadline(Token::Clock::now() - std::chrono::milliseconds(1));
  if (token.reason() != Reason::Deadline)
    TEST_FAILED("An expired deadline must cancel");

  const auto result = check_and_catch(token);
  if (result != Fmi::to_string(Reason::Deadline))
    TEST_FAILED("Unexpected exception: " + result);
  TEST_PASSED();
}

void probe()
{
  Token token;
  std::atomic<int> calls{0};
  std::atomic<bool> gone{false};
  token.setProbe(
      [&]()
      {
        ++calls;
        return gone ? Reason::Disconnected : Reason::None;
      },
      std::chrono::hours(1));

  // The first check polls, the following ones wait for the interval
  for (int i = 0; i < 100; i++)
    if (token.cancelled())
      TEST_FAILED("The probe has not cancelled yet");
  if (calls != 1)
    TEST_FAILED("The probe must be polled once per interval, polled " + std::to_string(calls) +
                " times");

  Token token2;
  token2.setProbe(
      [&]()
      {
        ++calls;
        return gone ? Reason::Disconnected : Reason::None;
      },
      std::chrono::seconds(0));
  gone = true;
  if (token2.reason() != Reason::Disconnected)
    TEST_FAILED("The probe must cancel");

  // A probe which throws does not cancel
  Token token3;
  token3.setProbe([]() -> Reason { throw std::runtime_error("probe failed"); },
                  std::chrono::seconds(0));
  if (token3.cancelled())
    TEST_FAILED("A failing probe must not cancel");
  TEST_PASSED();
}

void scope()
{
  if (Fmi::current_cancellation_token())
    TEST_FAILED("There must be no current token outside a scope");
  Fmi::check_cancelled();  // nothing to do

  auto outer = std::make_shared<Token>();
  auto inner = std::make_shared<Token>();
  {
    Fmi::CancellationScope outer_scope(outer);
    {
      Fmi::CancellationScope inner_scope(inner);
      if (Fmi::current_cancellation_token() != inner)
        TEST_FAILED("The inner token must be current");
      inner->cancel();
      try
      {
        Fmi::check_cancelled();
        TEST_FAILED("check_cancelled() must throw for a cancelled current token");
      }
      catch (const Fmi::Exception& e)
      {
        if (!Fmi::is_cancellation(e))
          TEST_FAILED("Expected a cancellation");
      }
    }
    if (Fmi::current_cancellation_token() != outer)
      TEST_FAILED("The outer token must be restored");
    Fmi::check_cancelled();  // the outer token is not cancelled

    // Other threads have no current token unless it is passed to them
    bool other_has_token = true;
    std::thread([&]() { other_has_token = !!Fmi::current_cancellation_token(); }).join();
    if (other_has_token)
      TEST_FAILED("Another thread must not see the token");
  }
  if (Fmi::current_cancellation_token())
    TEST_FAILED("The token must not be current after the scope");
  TEST_PASSED();
}

void wrapped()
{
  auto token = std::make_shared<Token>();
  token->cancel();
  Fmi::CancellationScope scope(token);
  try
  {
    try
    {
      Fmi::check_cancelled();
    }
    catch (...)
    {
      throw Fmi::Exception::Trace(BCP, "Operation failed!");
    }
  }
  catch (const Fmi::Exception& e)
  {
    if (!Fmi::is_cancellation(e))
      TEST_FAILED("A wrapped cancellation must be recognized");
    TEST_PASSED();
  }
  TEST_FAILED("Expected an exception");
}

void other_exception()
{
  Fmi::Exception e(BCP, "Something else");
  if (Fmi::is_cancellation(e))
    TEST_FAILED("Another exception must not be a cancellation");
  TEST_PASSED();
}

void threads()
{
  // Several threads share a token, all of them must stop
  auto token = std::make_shared<Token>();
  std::atomic<int> stopped{0};
  std::vector<std::thread> workers;
  for (int i = 0; i < 4; i++)
    workers.emplace_back(
        [&]()
        {
          Fmi::CancellationScope scope(token);
          try
          {
            while (true)
              Fmi::check_cancelled();
          }
          catch (const Fmi::Exception&)
          {
            ++stopped;
          }
        });
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  token->cancel();
  for (auto& worker : workers)
    worker.join();
  if (stopped != 4)
    TEST_FAILED("All threads must stop");
  TEST_PASSED();
}

// ----------------------------------------------------------------------
/*!
 * The actual test suite
 */
// ----------------------------------------------------------------------

class tests : public tframe::tests
{
  virtual const char* error_message_prefix() const { return "\n\t"; }

  void test(void)
  {
    TEST(not_cancelled);
    TEST(cancel);
    TEST(deadline);
    TEST(probe);
    TEST(scope);
    TEST(wrapped);
    TEST(other_exception);
    TEST(threads);
  }
};

}  // namespace CancellationTest

int main(void)
{
  std::cout << std::endl << "Cancellation tester" << std::endl << "===================" << std::endl;
  CancellationTest::tests t;
  return t.run();
}
