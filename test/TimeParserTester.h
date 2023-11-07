#include <functional>
#include <string>
#include <vector>
#include "DateTime.h"
#include "TypeName.h"
#include <regression/tframe.h>

namespace Fmi {
  namespace Test {

    template <typename ResponseType>
    struct TimeParseTest
    {
      std::string src;
      ResponseType expected;
    };

    bool check_time_parse(const std::vector<TimeParseTest<Fmi::DateTime> >& data,
                          std::function<Fmi::DateTime(const std::string&)> parser)
    {
      using Fmi::DateTime;
      int num_tests = 0;
      int num_passed = 0;
      for (const auto& item : data) {
        Fmi::DateTime result;
        num_tests++;
        try {
          result = parser(item.src);
          if (result != item.expected) {
            std::ostringstream msg;
            msg << "ERROR parsing '" << item.src << "': got " << to_simple_string(result)
                << ", expected " << to_simple_string(item.expected);
            TEST_FAILED(msg.str());
          }
          num_passed++;
        } catch (tframe::failed&) {
          throw;
        } catch (const std::exception& e) {
          TEST_FAILED("Failed to parse '" + item.src + "': got exception of type "
                      + Fmi::get_type_name(&e) + " - " + e.what());
        } catch (...) {
          TEST_FAILED("Failed to parse '" + item.src + "': got exception of type "
                      + Fmi::current_exception_type());
        }
      }
      return num_tests == num_passed;
    }

    bool check_time_parse_fail(const std::vector<std::string>& invalid,
                               std::function<Fmi::DateTime(const std::string&)> parser)
    {
      using Fmi::DateTime;
      int num_tests = 0;
      int num_passed = 0;
      for (const std::string& item : invalid) {
        Fmi::DateTime result;
        num_tests++;
        try {
          result = parser(item);
          TEST_FAILED("Should fail to parse '" + item + "': got " + to_simple_string(result));
        } catch (tframe::failed&) {
          throw;
        } catch (...) {
          num_passed++;
        }
      }
      return num_tests == num_passed;
    }
  }
}

