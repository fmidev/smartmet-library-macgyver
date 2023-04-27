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
    db::PostgreSQLConnectionOptions opts;
//    std::shared_ptr<db::PostgreSQLConnection> conn;
}

BOOST_AUTO_TEST_CASE(get_connection_options)
{
    std::ifstream config("PostgreSQLConnectionTest.conf");
    BOOST_REQUIRE(bool(config));

    config >> env::opts.host
           >> env::opts.port
           >> env::opts.database
           >> env::opts.username
           >> env::opts.password;
}

BOOST_AUTO_TEST_CASE(test_parametrized_sql_1, * utf::depends_on("get_connection_options"))
{
    db::PostgreSQLConnection conn(env::opts);
    BOOST_REQUIRE(conn.isConnected());
    pqxx::result result =
        SHOW_EXCEPTIONS(conn.exec_params(
                "SELECT id, lat, lon FROM geonames WHERE name=$1",
                "Valassaaret"));
    BOOST_REQUIRE_EQUAL(result.size(), std::size_t(1));
    int id = result[0]["id"].as<int>();
    BOOST_CHECK_EQUAL(id, 632561);
}

BOOST_AUTO_TEST_CASE(test_prepared_sql_1, * utf::depends_on("get_connection_options"))
{
    db::PostgreSQLConnection conn(env::opts);
    BOOST_REQUIRE(conn.isConnected());
    auto sql = SHOW_EXCEPTIONS(
        std::make_shared<db::PostgreSQLConnection::PreparedSQL>(
            conn,
            "test",
            "SELECT id, lat, lon FROM geonames WHERE name=$1"));

    pqxx::result result= SHOW_EXCEPTIONS(sql->exec("Valassaaret"));
    BOOST_REQUIRE_EQUAL(result.size(), std::size_t(1));
    int id = result[0]["id"].as<int>();
    BOOST_CHECK_EQUAL(id, 632561);
}

BOOST_AUTO_TEST_CASE(test_parametrized_sql_2, * utf::depends_on("get_connection_options"))
{
    db::PostgreSQLConnection conn(env::opts);
    BOOST_REQUIRE(conn.isConnected());
    auto transaction = conn.transaction();
    pqxx::result result =
        SHOW_EXCEPTIONS(
            conn.exec_params(
                "SELECT id, lat, lon FROM geonames WHERE name=$1 AND countries_iso2=$2",
                "Riga",
                "LV"));
    BOOST_REQUIRE_EQUAL(result.size(), std::size_t(2));
    int id = result[0]["id"].as<int>();
    BOOST_CHECK_EQUAL(id, 456172);
}

BOOST_AUTO_TEST_CASE(test_prepared_sql_reopen, * utf::depends_on("get_connection_options"))
{
    db::PostgreSQLConnection conn(env::opts);
    BOOST_REQUIRE(conn.isConnected());
    auto sql = SHOW_EXCEPTIONS(
        std::make_shared<db::PostgreSQLConnection::PreparedSQL>(
            conn,
            "test",
            "SELECT id, lat, lon FROM geonames WHERE name=$1"));

    pqxx::result result= SHOW_EXCEPTIONS(sql->exec("Valassaaret"));
    BOOST_REQUIRE_EQUAL(result.size(), std::size_t(1));
    int id = result[0]["id"].as<int>();
    BOOST_CHECK_EQUAL(id, 632561);

    // Reopen connection and check whether prepared SQL is restored

    conn.reopen();

    result= SHOW_EXCEPTIONS(sql->exec("Valassaaret"));
    BOOST_REQUIRE_EQUAL(result.size(), std::size_t(1));
    id = result[0]["id"].as<int>();
    BOOST_CHECK_EQUAL(id, 632561);
}
