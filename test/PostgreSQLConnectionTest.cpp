#include "PostgreSQLConnection.h"
#include "DebugTools.h"
#include <boost/test/included/unit_test.hpp>

#include <fstream>
#include <iostream>
#include <memory>

namespace utf = boost::unit_test;
namespace db = Fmi::Database;

utf::test_suite* init_unit_test_suite(int argc, char* argv[])
{
  const char* name = "PostgreSQLConnection tester";
  utf::unit_test_log.set_threshold_level(utf::log_messages);
  utf::framework::master_test_suite().p_name.value = name;
  BOOST_TEST_MESSAGE("");
  BOOST_TEST_MESSAGE(name);
  BOOST_TEST_MESSAGE(std::string(std::strlen(name), '='));
  return NULL;
}

namespace env
{
    std::shared_ptr<db::PostgreSQLConnection> conn;
}

BOOST_AUTO_TEST_CASE(test_open_connection)
{
    std::ifstream config("PostgreSQLConnectionTest.conf");
    BOOST_REQUIRE(bool(config));

    db::PostgreSQLConnectionOptions opts;
    config >> opts.host >> opts.port >> opts.database >> opts.username >> opts.password;

    env::conn = std::make_shared<db::PostgreSQLConnection>(opts);
    BOOST_REQUIRE(static_cast<bool>(env::conn));
    BOOST_REQUIRE(env::conn->isConnected());
}

BOOST_AUTO_TEST_CASE(test_parametrized_sql_1, * utf::depends_on("test_open_connection"))
{
    pqxx::result result =
        SHOW_EXCEPTIONS(env::conn->exec_params(
                "SELECT id, lat, lon FROM geonames WHERE name=$1",
                "Valassaaret"));
    BOOST_REQUIRE_EQUAL(result.size(), std::size_t(1));
    int id = result[0]["id"].as<int>();
    BOOST_CHECK_EQUAL(id, 632561);
}

BOOST_AUTO_TEST_CASE(test_prepared_sql_1, * utf::depends_on("test_open_connection"))
{
    BOOST_REQUIRE(env::conn->isConnected());
    auto sql = SHOW_EXCEPTIONS(
        std::make_shared<db::PostgreSQLConnection::PreparedSQL>(
            *env::conn,
            "test",
            "SELECT id, lat, lon FROM geonames WHERE name=$1"));

    pqxx::result result= SHOW_EXCEPTIONS(sql->exec("Valassaaret"));
    BOOST_REQUIRE_EQUAL(result.size(), std::size_t(1));
    int id = result[0]["id"].as<int>();
    BOOST_CHECK_EQUAL(id, 632561);
}

BOOST_AUTO_TEST_CASE(test_parametrized_sql_2, * utf::depends_on("test_open_connection"))
{
    pqxx::result result =
        SHOW_EXCEPTIONS(
            env::conn->exec_params(
                "SELECT id, lat, lon FROM geonames WHERE name=$1",
                "Riga"));
    BOOST_REQUIRE_EQUAL(result.size(), std::size_t(2));
    int id = result[0]["id"].as<int>();
    BOOST_CHECK_EQUAL(id, 456172);
}
