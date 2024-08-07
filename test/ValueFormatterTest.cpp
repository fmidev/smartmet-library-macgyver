// ======================================================================
/*!
 * \file
 * \brief Regression tests for namespace ValueFormatter
 */
// ======================================================================

#include "ValueFormatter.h"
#include <regression/tframe.h>
#include <cmath>
#include <limits>
#include <sstream>

template <typename T>
std::string tostr(const T& theValue)
{
  std::ostringstream out;
  out << theValue;
  return out.str();
}

//! Protection against conflicts with global functions
namespace ValueFormatterTest
{
// ----------------------------------------------------------------------

void simple()
{
  Fmi::ValueFormatterParam opt;
  opt.floatField = "none";
  Fmi::ValueFormatter fmt(opt);

  int precision = -1;

  std::string result;

  if ((result = fmt.format(1, precision)) != "1")
    TEST_FAILED("Formatting 1 failed: " + result);

  if ((result = fmt.format(1.23, precision)) != "1.23")
    TEST_FAILED("Formatting 1.23 failed: " + result);

  if ((result = fmt.format(-1.23, precision)) != "-1.23")
    TEST_FAILED("Formatting -1.23 failed: " + result);

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void nan()
{
  std::string result;
  int precision = -1;

  const float nan = std::numeric_limits<float>::quiet_NaN();

  {
    Fmi::ValueFormatterParam opt;
    opt.floatField = "none";
    Fmi::ValueFormatter fmt(opt);
    if ((result = fmt.format(nan, precision)) != "nan")
      TEST_FAILED("Failed to return 'nan' for NaN value, result = " + result);
  }

  {
    Fmi::ValueFormatterParam opt;
    opt.floatField = "none";
    opt.missingText = "nullptr";
    Fmi::ValueFormatter fmt(opt);
    if ((result = fmt.format(nan, precision)) != "nullptr")
      TEST_FAILED("Failed to return 'nullptr' for NaN value, result = " + result);
  }

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void floatfield()
{
  std::string result;
  int precision = -1;

  {
    Fmi::ValueFormatterParam opt;
    opt.floatField = "fixed";
    Fmi::ValueFormatter fmt(opt);

    if ((result = fmt.format(1, precision)) != "1")
      TEST_FAILED("Formatting 1 failed: " + result);

    if ((result = fmt.format(1.23, precision)) != "1.23")
      TEST_FAILED("Formatting 1.23 failed: " + result);

    if ((result = fmt.format(-1.23, precision)) != "-1.23")
      TEST_FAILED("Formatting -1.23 failed: " + result);
  }

  {
    Fmi::ValueFormatterParam opt;
    opt.floatField = "scientific";
    Fmi::ValueFormatter fmt(opt);

    if ((result = fmt.format(1, precision)) != "1e+0")
      TEST_FAILED("Formatting 1 failed: " + result);

    if ((result = fmt.format(1.23, precision)) != "1.23e+0")
      TEST_FAILED("Formatting 1.23 failed: " + result);

    if ((result = fmt.format(-1.23, precision)) != "-1.23e+0")
      TEST_FAILED("Formatting -1.23 failed: " + result);

    if ((result = fmt.format(2.3e-2, 1)) != "2.3e-2")
      TEST_FAILED("Formatting 2.3e-2 failed: " + result);
  }

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void precision()
{
  std::string result;

  {
    Fmi::ValueFormatterParam opt;
    opt.floatField = "none";
    Fmi::ValueFormatter fmt(opt);

    int precision = 2;

    if ((result = fmt.format(1, precision)) != "1")
      TEST_FAILED("Formatting 1 failed: " + result);

    if ((result = fmt.format(1.23, precision)) != "1.2")
      TEST_FAILED("Formatting 1.23 failed: " + result);

    if ((result = fmt.format(-1.23, precision)) != "-1.2")
      TEST_FAILED("Formatting -1.23 failed: " + result);

    if ((result = fmt.format(1.25, precision)) != "1.3")
      TEST_FAILED("Formatting 1.25 failed: " + result);

    if ((result = fmt.format(-1.25, precision)) != "-1.3")
      TEST_FAILED("Formatting -1.25 failed: " + result);

    if ((result = fmt.format(0.10001, precision)) != "0.1")
      TEST_FAILED("Formatting 0.10001 failed: " + result);
  }

  {
    Fmi::ValueFormatterParam opt;
    opt.floatField = "fixed";
    Fmi::ValueFormatter fmt(opt);

    int precision = 2;

    if ((result = fmt.format(1, precision)) != "1.00")
      TEST_FAILED("Formatting 1 failed: " + result);

    if ((result = fmt.format(1.23, precision)) != "1.23")
      TEST_FAILED("Formatting 1.23 failed: " + result);

    if ((result = fmt.format(-1.23, precision)) != "-1.23")
      TEST_FAILED("Formatting -1.23 failed: " + result);

    if ((result = fmt.format(5.555, precision)) != "5.56")
      TEST_FAILED("Formatting 5.555 failed: " + result);

    if ((result = fmt.format(5.599, precision)) != "5.60")
      TEST_FAILED("Formatting 5.599 failed: " + result);
  }

  {
    Fmi::ValueFormatterParam opt;
    opt.floatField = "scientific";
    Fmi::ValueFormatter fmt(opt);

    int precision = 2;

    if ((result = fmt.format(1, precision)) != "1.00e+0")
      TEST_FAILED("Formatting 1 failed: " + result);

    if ((result = fmt.format(1.23, precision)) != "1.23e+0")
      TEST_FAILED("Formatting 1.23 failed: " + result);

    if ((result = fmt.format(-1.23, precision)) != "-1.23e+0")
      TEST_FAILED("Formatting -1.23 failed: " + result);

    if ((result = fmt.format(5.555, precision)) != "5.55e+0")
      TEST_FAILED("Formatting 5.555 failed: " + result);
  }

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
    TEST(simple);
    TEST(nan);
    TEST(precision);
    TEST(floatfield);
  }
};

}  // namespace ValueFormatterTest

//! The main program
int main(void)
{
  using namespace std;
  cout << endl << "ValueFormatter tester" << endl << "=====================" << endl;
  ValueFormatterTest::tests t;
  return t.run();
}

// ======================================================================
