#include "DebugTools.h"
#include <regression/tframe.h>
#include <fstream>
#include <memory>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

namespace ba = boost::algorithm;

std::vector<std::string> parse_stack_trace(const std::string& input)
{
  static boost::regex r_ansi("\\033\[[0-9;]+m");

  std::vector<std::string> lines;
  ba::split(
      lines,
      boost::regex_replace(input, r_ansi, "", boost::match_default | boost::format_all),
      ba::is_any_of("\n"));

  std::vector<std::string> result;
  for (const auto& line : lines) {
      if (line.substr(0, 9) == "EXCEPTION") {
          result.push_back(ba::trim_copy(line.substr(10)));
      }
  }
  return result;
}

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

    std::string output = out.str();
    const auto tmp = parse_stack_trace(output);
    if (tmp.size() != 2) {
        TEST_FAILED("Exactly 2 frames expected");
    }
    //std::cout << out.str();

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
    const auto tmp = parse_stack_trace(output);
    if (tmp.size() != 2) {
        TEST_FAILED("Exactly 2 frames expected");
    }
    //std::cout << out.str();

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
            e.disableLogging();
            throw e;
        }
    } catch (...) {
        auto e = Fmi::Exception::Trace(BCP, "Operation failed");
        e.addParameter("Depth", std::to_string(depth));
        e.disableStackTrace();
        e.disableLogging();
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
    const auto tmp = parse_stack_trace(output);
    if (tmp.size() != 8) {
        TEST_FAILED("Exactly 8 frames expected. Got " + std::to_string(tmp.size()));
    }
    //std::cout << out.str();

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

