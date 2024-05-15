// ======================================================================
/*!
 * \file
 * \brief Regression tests for Fmi::PostgreSQLConnectionOptions
 */
// ======================================================================

#include "PostgreSQLConnection.h"
#include "TypeName.h"
#include <regression/tframe.h>
#include <string>

namespace PostgreSQLConnectionOptionsTest
{
void parse_options_1()
{
  const std::string conn_str = "host=db.example.com dbname=example port=8001 user=foo password=bar";
  Fmi::Database::PostgreSQLConnectionOptions opt(conn_str);
  if (opt.host != "db.example.com")
  {
    TEST_FAILED("opt.host='" + opt.host + "' but not expected 'db.example.com'");
  }
  if (opt.database != "example")
  {
    TEST_FAILED("opt.database='" + opt.database + "' but not expected 'example'");
  }
  if (opt.username != "foo")
  {
    TEST_FAILED("opt.username='" + opt.username + "' but not expected 'foo'");
  }
  if (opt.password != "bar")
  {
    TEST_FAILED("opt.password='" + opt.password + "' but not expected 'bar'");
  }
  if (opt.port != 8001)
  {
    TEST_FAILED("opt.port='" + std::to_string(opt.port) +
                "' but not expected 8001");
  }
  if (opt.encoding != "UTF8")
  {
    TEST_FAILED("opt.encoding='" + opt.encoding + "' but not expected 'UTF8'");
  }
  if (opt.connect_timeout != 0)
  {
    TEST_FAILED("opt.connect_timeout=" + std::to_string(opt.connect_timeout) +
                "' but not expected 0");
  }

  const std::string new_conn_str = opt;
  if (new_conn_str != conn_str)
  {
    TEST_FAILED("'" + new_conn_str + "' <=> '" + conn_str + "'");
  }

  TEST_PASSED();
}

class tests : public tframe::tests
{
  virtual const char* error_message_prefix() const { return "\n\t"; }
  void test(void) { TEST(parse_options_1); }
};

}  // namespace PostgreSQLConnectionOptionsTest

//! The main program
int main(void)
{
  using namespace std;
  cout << endl << "PostgreSQLConnectionOptionsTest" << endl << "==========" << endl;
  PostgreSQLConnectionOptionsTest::tests t;
  return t.run();
}
