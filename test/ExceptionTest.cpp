#include "Exception.h"
#include <regression/tframe.h>
#include <macgyver/DebugTools.h>
#include <future>
#include <iostream>
#include <sstream>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

using std::string;

namespace ba = boost::algorithm;

#define TEST_FAILED_UNLESS(pred)             \
  if (!(pred))                               \
  {                                          \
    TEST_FAILED("Check '" #pred "' failed"); \
  }

// Protection against conflicts with global functions
namespace ExceptionTest
{
// ----------------------------------------------------------------------

void test_std_library_exception_reporting_1()
{
  try
  {
    try
    {
      throw std::runtime_error("Foo");
    }
    catch (...)
    {
      throw Fmi::Exception::Trace(BCP, "Bar");
    }
    TEST_FAILED("Exception should have been thrown");
  }
  catch (const Fmi::Exception& e)
  {
    TEST_FAILED_UNLESS(e.getWhat() == std::string("Bar"));
    const Fmi::Exception* prev = e.getPrevException();
    TEST_FAILED_UNLESS(prev != NULL);
    TEST_FAILED_UNLESS(prev->getWhat() == std::string("[Runtime error] Foo"));
  }
  catch (...)
  {
    TEST_FAILED("Fmi::Exception was expected");
  }
  TEST_PASSED();
}

int count_lines(const std::string& input, const std::string& regex)
{
  static boost::regex r_ansi("\\033\[[0-9;]+m");

  std::vector<std::string> lines;
  ba::split(lines, input, ba::is_any_of("\n"));

  int count = 0;
  boost::regex r(regex);
  for (const auto& line : lines) {
      std::string in = boost::regex_replace(line, r_ansi, "", boost::match_default | boost::format_all);
      if (boost::regex_search(in, r)) {
          count++;
      }
  }
  return count;
}

struct MyRuntimeError : public std::runtime_error
{
  MyRuntimeError() : std::runtime_error("My runtime error") {}
};

void test_std_library_exception_reporting_2()
{
  try
  {
    try
    {
      throw MyRuntimeError();
    }
    catch (...)
    {
      throw Fmi::Exception::Trace(BCP, "Bar");
    }
    TEST_FAILED("Exception should have been thrown");
  }
  catch (const Fmi::Exception& e)
  {
    TEST_FAILED_UNLESS(e.getWhat() == std::string("Bar"));
    const Fmi::Exception* prev = e.getPrevException();
    TEST_FAILED_UNLESS(prev != NULL);
    TEST_FAILED_UNLESS(prev->getWhat() ==
                       std::string("[ExceptionTest::MyRuntimeError] My runtime error"));
  }
  catch (...)
  {
    TEST_FAILED("Fmi::Exception was expected");
  }
  TEST_PASSED();
}

struct MyException1
{
};

void test_other_exception_reporting_1()
{
  try
  {
    try
    {
      throw MyException1();
    }
    catch (...)
    {
      throw Fmi::Exception::Trace(BCP, "Bar");
    }
    TEST_FAILED("Exception should have been thrown");
  }
  catch (const Fmi::Exception& e)
  {
    TEST_FAILED_UNLESS(e.getWhat() == std::string("Bar"));
    const Fmi::Exception* prev = e.getPrevException();
    TEST_FAILED_UNLESS(prev != NULL);
    TEST_FAILED_UNLESS(prev->getWhat() == std::string("[ExceptionTest::MyException1]"));
  }
  catch (...)
  {
    TEST_FAILED("Fmi::Exception was expected");
  }
  TEST_PASSED();
}

void test_funct_1()
{
  throw Fmi::Exception::Trace(BCP, "Some error");
}

void test_funct_2()
{
  try
  {
    test_funct_1();
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Failed");
  }
}

// Test compatibility of Fmi::Exception with std::future
void throw_fmi_exception_in_async_call()
{
  std::future<void> f = std::async(std::launch::async, &test_funct_1);
  try
  {
    f.get();
    TEST_FAILED("Exception must be thrown");
  }
  catch (const Fmi::Exception& e)
  {
    if (e.getWhat() != std::string("Some error"))
    {
      TEST_FAILED("Unexpected value of exception message");
    }
    const Fmi::Exception* prev = e.getPrevException();
    if (prev)
    {
      TEST_FAILED("There should be no previous exception");
    }
    // std::cout << "f.get() -> exception:\n";
    // e.printError();
    TEST_PASSED();
  }
  catch (...)
  {
    TEST_FAILED("Unexpected exception type");
  }
}

void throw_nested_fmi_exception_in_async_call()
{
  std::future<void> f = std::async(std::launch::async, &test_funct_2);
  try
  {
    f.get();
    TEST_FAILED("Exception must be thrown");
  }
  catch (const Fmi::Exception& e)
  {
    // std::cout << "f.get() -> exception:\n";
    // e.printError();
    if (e.getWhat() != std::string("Failed"))
    {
      TEST_FAILED("Unexpected value of exception message");
    }
    const Fmi::Exception* prev = e.getPrevException();
    if (prev)
    {
      if (prev->getWhat() != std::string("Some error"))
      {
        TEST_FAILED("Unexpected value of exception message");
      }
      prev = prev->getPrevException();
      if (prev)
      {
        TEST_FAILED("There should be only 2 exception levels");
      }
    }
    else
    {
      TEST_FAILED("There should be no previous exception");
    }
    TEST_PASSED();
  }
}

void test_squashing_stack_trace()
{
    bool catched = false;
    try
    {
        try
        {
            try
            {
                Fmi::Exception exc(BCP, "Test exception");
                exc.addParameter("test", "42");
                throw exc;
            }
            catch (...)
            {
                throw Fmi::Exception::Trace(BCP, "rethrowing 1");
            }
        }
        catch (...)
        {
            throw Fmi::Exception::Trace(BCP, "rethrowing 2");
        }
    }
    catch (const Fmi::Exception& e)
    {
        auto e1 = Fmi::Exception::SquashTrace(BCP, "Testing");
        const std::string what = e1.getWhat();
        if (what != "Test exception")
        {
            TEST_FAILED("Expected exception message 'Test exception', got '" + what + "'");
        }
        const char* p = e1.getParameterValue("test");
        if (!p || p  != std::string("42"))
        {
            TEST_FAILED("parameter 'test' is expected to exist and have value '42'");
        }
        catched = true;
    }
    if (!catched) {
        TEST_FAILED("Expected to get an exception");
    }
    TEST_PASSED();
}

void rethrow_with_stack_trace_disabled_example()
{
   try
   {
     try
     {
       try
       {
         Fmi::Exception exc(BCP, "Test exception");
         exc.addParameter("test", "42");
         exc.disableStackTrace();
         throw exc;
       }
       catch (...)
       {
         throw Fmi::Exception::Trace(BCP, "rethrowing 1");
       }
     }
     catch (...)
     {
       throw Fmi::Exception::Trace(BCP, "rethrowing 2");
     }
   }
   catch (const Fmi::Exception& e)
   {
     throw Fmi::Exception::Trace(BCP, "rethrowing 3");
   }
}


void test_stack_trace_disabled_1()
{
  std::ostringstream output;
  try
  {
    rethrow_with_stack_trace_disabled_example();
  }
  catch (const Fmi::Exception& e)
  {
    if (!e.stackTraceDisabled())
    {
      TEST_FAILED("Stack trace was expected to be disabled");
    }
    const auto redirect = std::make_shared<Fmi::Redirecter>(output, std::cout);
    output << e;
  }

  //std::cout << output.str() << std::endl;
  int count = count_lines(output.str(), "^EXCEPTION\\ rethrowing");
  if (count != 0) {
      TEST_FAILED("Stack trace was expected to be hidden");
  }
  TEST_PASSED();
}

void test_stack_trace_disabled_2()
{
  std::string output;
  try
  {
    rethrow_with_stack_trace_disabled_example();
  }
  catch (const Fmi::Exception& e)
  {
    if (!e.stackTraceDisabled())
    {
      TEST_FAILED("Stack trace was expected to be disabled");
    }
    output = e.getStackTrace();
  }

  //std::cout << output << std::endl;
  int count = count_lines(output, "EXCEPTION\\ rethrowing");
  if (count != 0) {
      TEST_FAILED("Stack trace was expected to be hidden");
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
    TEST(test_std_library_exception_reporting_1);
    TEST(test_std_library_exception_reporting_2);
    TEST(test_other_exception_reporting_1);
    TEST(throw_fmi_exception_in_async_call);
    TEST(throw_nested_fmi_exception_in_async_call);
    TEST(test_squashing_stack_trace);
    TEST(test_stack_trace_disabled_1);
    TEST(test_stack_trace_disabled_2);
  }
};

}  // namespace ExceptionTest

//! The main program
int main(void)
{
  using namespace std;
  cout << endl << "Exception tester" << endl << "=============" << endl;
  ExceptionTest::tests t;
  return t.run();
}
