#include "DebugTools.h"
#include <regression/tframe.h>
#include <fstream>

int redirecter_test()
{
    std::ostringstream part;
    std::ofstream output("/dev/null");
    output << "some output 1\n";
    {
        Fmi::Redirecter redirect(part, output);
        output << "test";
    }
    output << "some more output\n";
    output << std::flush;
    if (part.str() != "test") {
        TEST_FAILED("Expected 'test', but got '" + part.str() + "'");
    }
    TEST_PASSED();
}


class tests : public tframe::tests
{
  virtual const char* error_message_prefix() const { return "\n\t"; }

  void test(void)
  {
    TEST(redirecter_test);
  }
};

int main(void)
{
  std::cout << std::endl
            << "Debug Tools tester" << std::endl
            << "=====================" << std::endl;
  tests t;
  return t.run();
}

