#include "DebugTools.h"
#include <regression/tframe.h>
#include <fstream>
#include <memory>

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

int show_exceptions_test_1()
{
    std::ostringstream out;

    const auto funct = [] () -> int { throw std::runtime_error("Testing"); };
    auto redirecter = std::make_shared<Fmi::Redirecter>(out, std::cout);
    try {
        (void) SHOW_EXCEPTIONS(funct());
        TEST_FAILED("Excepted std::runtime_error thrown by previous statement");
    } catch (...) {}
    redirecter.reset();

    const std::string output = out.str();
    // FIXME: add tests for output value
    std::cout << out.str();

    TEST_PASSED();
}

int show_exceptions_test_2()
{
    std::ostringstream out;

    const auto funct = [] () -> int {
        throw std::move(Fmi::Exception(BCP, "Testing").addParameter("TestName", "TestValue")); };
    auto redirecter = std::make_shared<Fmi::Redirecter>(out, std::cout);
    try {
        (void) SHOW_EXCEPTIONS(funct());
        TEST_FAILED("Excepted std::runtime_error thrown by previous statement");
    } catch (...) {}
    redirecter.reset();

    const std::string output = out.str();
    // FIXME: add tests for output value
    std::cout << out.str();

    TEST_PASSED();
}

int show_exceptions_test_3_funct(int depth)
{
    try {
        if (depth > 0) {
            return show_exceptions_test_3_funct(depth - 1);
        } else {
            Fmi::Exception e(BCP, "Testing");
            e.disableStackTrace();
            throw e;
        }
    } catch (...) {
        auto e = Fmi::Exception::Trace(BCP, "Operation failed");
        e.addParameter("Depth", std::to_string(depth));
        e.disableStackTrace();
        throw e;
    }
}

int show_exceptions_test_3()
{
    std::ostringstream out;

    auto redirecter = std::make_shared<Fmi::Redirecter>(out, std::cout);
    try {
        (void) SHOW_EXCEPTIONS(show_exceptions_test_3_funct(5));
        TEST_FAILED("Excepted std::runtime_error thrown by previous statement");
    } catch (...) {}
    redirecter.reset();

    const std::string output = out.str();
    // FIXME: add tests for output value
    std::cout << out.str();

    TEST_PASSED();
}

class tests : public tframe::tests
{
  virtual const char* error_message_prefix() const { return "\n\t"; }

  void test(void)
  {
    TEST(redirecter_test);
    TEST(show_exceptions_test_1);
    TEST(show_exceptions_test_2);
    TEST(show_exceptions_test_3);
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

