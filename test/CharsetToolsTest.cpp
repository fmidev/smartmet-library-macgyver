// ======================================================================
/*!
 * \file
 * \brief Regression tests for CharsetTools
 */
// ======================================================================

#include "CharsetTools.h"
#include <regression/tframe.h>
#include <string>

using std::string;

// Protection against conflicts with global functions
namespace CharsetToolsTest
{
// ----------------------------------------------------------------------

void toupper()
{
  if (Fmi::toupper('a') != 'A') TEST_FAILED("Failed to convert a to A");
  if (Fmi::toupper('A') != 'A') TEST_FAILED("Failed to convert A to A");
  if (Fmi::toupper('ö') != (unsigned char)'Ö') TEST_FAILED("Failed to convert ö to Ö");
  if (Fmi::toupper('1') != '1') TEST_FAILED("Failed to convert 1 to 1");

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void tolower()
{
  if (Fmi::tolower('A') != 'a') TEST_FAILED("Failed to convert A to a");
  if (Fmi::tolower('a') != 'a') TEST_FAILED("Failed to convert a to a");
  if (Fmi::tolower('Ö') != (unsigned char)'ö') TEST_FAILED("Failed to convert ö to Ö");
  if (Fmi::tolower('1') != '1') TEST_FAILED("Failed to convert 1 to 1");

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void tonordic() { TEST_NOT_IMPLEMENTED(); }
// ----------------------------------------------------------------------

void tolowernordic() { TEST_NOT_IMPLEMENTED(); }
// ----------------------------------------------------------------------

void utf8_to_latin1()
{
  string res;
  if ((res = Fmi::utf8_to_latin1("Janakkala")) != "Janakkala")
    TEST_FAILED("Failed on string 'Janakkala': " + res);

  if ((res = Fmi::utf8_to_latin1("Ã„Ã¤nekoski")) != "Äänekoski")
    TEST_FAILED("Failed on string 'Äänekoski': " + res);

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void latin1_to_utf8()
{
  string res;
  if ((res = Fmi::latin1_to_utf8("Janakkala")) != "Janakkala")
    TEST_FAILED("Failed on string 'Janakkala': " + res);

  if ((res = Fmi::latin1_to_utf8("Äänekoski")) != "Ã„Ã¤nekoski")
    TEST_FAILED("Failed on string 'Äänekoski': " + res);

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void utf16_to_utf8() { TEST_NOT_IMPLEMENTED(); }
// ----------------------------------------------------------------------

void utf8_to_utf16() { TEST_NOT_IMPLEMENTED(); }
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
    TEST(toupper);
    TEST(tolower);
    TEST(tonordic);
    TEST(tolowernordic);
    TEST(utf8_to_latin1);
    TEST(latin1_to_utf8);
    TEST(utf16_to_utf8);
    TEST(utf8_to_utf16);
  }
};

}  // namespace CharsetToolsTest

//! The main program
int main(void)
{
  using namespace std;
  cout << endl << "CharsetTools tester" << endl << "===================" << endl;
  CharsetToolsTest::tests t;
  return t.run();
}

// ======================================================================
