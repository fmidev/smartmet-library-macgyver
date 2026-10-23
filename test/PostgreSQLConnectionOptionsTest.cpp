// ======================================================================
/*!
 * \file
 * \brief Regression tests for Fmi::PostgreSQLConnectionOptions
 */
// ======================================================================

#include "PostgreSQLConnection.h"
#include "TypeName.h"
#include <boost/date_time/posix_time/posix_time.hpp>
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
    TEST_FAILED("opt.port='" + boost::lexical_cast<std::string>(opt.port) +
                "' but not expected 8001");
  }
  if (opt.encoding != "UTF8")
  {
    TEST_FAILED("opt.encoding='" + opt.encoding + "' but not expected 'UTF8'");
  }
  if (opt.connect_timeout != 0)
  {
    TEST_FAILED("opt.connect_timeout=" + boost::lexical_cast<std::string>(opt.connect_timeout) +
                "' but not expected 0");
  }

  const std::string new_conn_str = opt;
  if (new_conn_str != conn_str)
  {
    TEST_FAILED("'" + new_conn_str + "' <=> '" + conn_str + "'");
  }

  TEST_PASSED();
}

void parse_dead_connection_detection_options()
{
  const std::string conn_str =
      "host=db.example.com dbname=example port=8001 user=foo password=bar"
      " tcp_user_timeout=30000 keepalives_idle=60 keepalives_interval=10 keepalives_count=3";
  Fmi::Database::PostgreSQLConnectionOptions opt(conn_str);
  if (opt.tcp_user_timeout != 30000)
    TEST_FAILED("opt.tcp_user_timeout=" + std::to_string(opt.tcp_user_timeout) +
                " but not expected 30000");
  if (opt.keepalives_idle != 60)
    TEST_FAILED("opt.keepalives_idle=" + std::to_string(opt.keepalives_idle) +
                " but not expected 60");
  if (opt.keepalives_interval != 10)
    TEST_FAILED("opt.keepalives_interval=" + std::to_string(opt.keepalives_interval) +
                " but not expected 10");
  if (opt.keepalives_count != 3)
    TEST_FAILED("opt.keepalives_count=" + std::to_string(opt.keepalives_count) +
                " but not expected 3");

  const std::string new_conn_str = opt;
  if (new_conn_str != conn_str)
    TEST_FAILED("'" + new_conn_str + "' <=> '" + conn_str + "'");

  // Unset (0) values are left out of the connection string, leaving libpq defaults in place
  Fmi::Database::PostgreSQLConnectionOptions defaults(
      "host=db.example.com dbname=example port=8001 user=foo password=bar");
  if (defaults.tcp_user_timeout != 0 || defaults.keepalives_idle != 0 ||
      defaults.keepalives_interval != 0 || defaults.keepalives_count != 0)
    TEST_FAILED("dead connection detection options not 0 by default");

  TEST_PASSED();
}

void hide_password_in_exception()
{
  const std::map<std::string, std::string> testcases{
    // password at the beginning
    {"password=mypassword host=db.example.com dbname=example port=fail user=foo",
     "Failed to parse connection string 'password=*** host=db.example.com dbname=example port=fail user=foo'"},
    // password at the end
    {"host=db.example.com dbname=example port=fail user=foo password=mypassword",
     "Failed to parse connection string 'host=db.example.com dbname=example port=fail user=foo password=***'"},
    // password in the middle
    {"host=db.example.com dbname=example port=fail  password=mypassword\tuser=foo",
     "Failed to parse connection string 'host=db.example.com dbname=example port=fail  password=***\tuser=foo'"},
    // password-like username, should be left as is
    {"host=db.example.com dbname=example port=fail user=foopassword=mypassword",
     "Failed to parse connection string 'host=db.example.com dbname=example port=fail user=foopassword=mypassword'"}
  };

  for (const auto& [input, expected] : testcases)
  {
    try
    {
      Fmi::Database::PostgreSQLConnectionOptions opt(input);
    }
    catch (const Fmi::Exception& t)
    {
      const std::string what{t.getWhat()};
      if (what != expected)
      {
        TEST_FAILED("'" + what + "' <=> '" + expected + "'");
      }
      else continue;
    }
    TEST_FAILED("expected exception");
  }
  TEST_PASSED();
}

void hide_password_in_toString()
{
  const std::string input{"password=mypassword host=db.example.com dbname=example port=1234 user=foo"};
  Fmi::Database::PostgreSQLConnectionOptions opts{input};
  {
    const auto str{opts.toString(true)};
    const std::string expected_without_pw{"host=db.example.com dbname=example port=1234 user=foo password=***"};    
    if (str != expected_without_pw)
    {
      TEST_FAILED("'" + str + "' <=> '" + expected_without_pw + "'");
    }
  }
  {
    const auto str{opts.toString(false)};
    const std::string expected_with_pw{"host=db.example.com dbname=example port=1234 user=foo password=mypassword"};
    if (str != expected_with_pw)
    {
      TEST_FAILED("'" + str + "' <=> '" + expected_with_pw + "'");
    }
  }
  TEST_PASSED();
}

void id_format()
{
  using Fmi::Database::PostgreSQLConnectionId;

  PostgreSQLConnectionId id{"db.example.com", 5432, "example"};

  if (fmt::format("{}", id) != "db.example.com:5432/example")
    TEST_FAILED("fmt::format(\"{}\", id) failed");

  if (fmt::format("{:h}", id) != "db.example.com")
    TEST_FAILED("fmt::format(\"{:h}\", id) failed");

  if (fmt::format("{:p}", id) != "5432")
    TEST_FAILED("fmt::format(\"{:p}\", id) failed");

  if (fmt::format("{:d}", id) != "example")
    TEST_FAILED("fmt::format(\"{:d}\", id) failed");

  TEST_PASSED();
}

class tests : public tframe::tests
{
  virtual const char* error_message_prefix() const { return "\n\t"; }
  void test(void)
  {
    TEST(parse_options_1);
    TEST(parse_dead_connection_detection_options);
    TEST(hide_password_in_exception);
    TEST(hide_password_in_toString);
    TEST(id_format);
  }
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
