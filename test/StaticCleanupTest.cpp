#include "StaticCleanup.h"

#include <regression/tframe.h>
#include <iostream>
#include <type_traits>
#include <vector>

using namespace std;
using Fmi::StaticCleanup;

// Copy and move must be disabled: a StaticCleanup registers exactly one cleanup
// function, and an AtExit's instance count is balanced by ctor/dtor pairs.
// Copying or moving either would break those invariants, so verify at compile
// time that the deletions are in place.
static_assert(!std::is_copy_constructible<StaticCleanup>::value,
              "StaticCleanup must not be copy constructible");
static_assert(!std::is_move_constructible<StaticCleanup>::value,
              "StaticCleanup must not be move constructible");
static_assert(!std::is_copy_assignable<StaticCleanup>::value,
              "StaticCleanup must not be copy assignable");
static_assert(!std::is_move_assignable<StaticCleanup>::value,
              "StaticCleanup must not be move assignable");
static_assert(!std::is_copy_constructible<StaticCleanup::AtExit>::value,
              "AtExit must not be copy constructible");
static_assert(!std::is_move_constructible<StaticCleanup::AtExit>::value,
              "AtExit must not be move constructible");

namespace StaticCleanupTest
{
// Cleanups run in the reverse order they were registered, and only when the
// AtExit guard is destroyed (not before).
void reverse_order()
{
  std::vector<int> order;
  {
    StaticCleanup c1([&order]() { order.push_back(1); });
    StaticCleanup c2([&order]() { order.push_back(2); });
    StaticCleanup c3([&order]() { order.push_back(3); });

    if (!order.empty())
      TEST_FAILED("Cleanup functions ran before the AtExit guard was destroyed");

    {
      StaticCleanup::AtExit guard;
    }  // cleanups run here, in reverse registration order

    if (order != std::vector<int>({3, 2, 1}))
      TEST_FAILED("Cleanup functions did not run in reverse registration order");
  }
  TEST_PASSED();
}

// Cleanups run at most once: a second AtExit create/destroy cycle must not
// re-run already-invoked cleanups (the registry is emptied after running).
void runs_at_most_once()
{
  int count = 0;
  {
    StaticCleanup c([&count]() { ++count; });

    {
      StaticCleanup::AtExit guard;
    }
    if (count != 1)
      TEST_FAILED("Cleanup did not run exactly once on first AtExit destruction");

    {
      StaticCleanup::AtExit guard;
    }
    if (count != 1)
      TEST_FAILED("Cleanup re-ran on a second AtExit cycle");
  }
  TEST_PASSED();
}

// Nested AtExit guards run cleanups only when the outermost one is destroyed.
void nested_guards()
{
  int count = 0;
  {
    StaticCleanup c([&count]() { ++count; });

    StaticCleanup::AtExit outer;
    {
      StaticCleanup::AtExit inner;
    }  // inner destroyed: outer still alive, cleanups must not run yet

    if (count != 0)
      TEST_FAILED("Cleanups ran when an inner (non-last) AtExit was destroyed");
  }  // outer destroyed here: cleanups run

  if (count != 1)
    TEST_FAILED("Cleanups did not run when the outermost AtExit was destroyed");

  TEST_PASSED();
}

class tests : public tframe::tests
{
  virtual const char* error_message_prefix() const { return "\n\t"; }

  void test()
  {
    TEST(reverse_order);
    TEST(runs_at_most_once);
    TEST(nested_guards);
  }
};
}  // namespace StaticCleanupTest

int main()
{
  using namespace std;
  cout << endl << "StaticCleanup" << endl << "=============" << endl;
  StaticCleanupTest::tests t;
  return t.run();
}
