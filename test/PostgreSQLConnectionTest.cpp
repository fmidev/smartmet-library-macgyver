#include "PostgreSQLConnection.h"
#include "DebugTools.h"
#include <boost/test/included/unit_test.hpp>

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <future>

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
  return nullptr;
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
            "test1",
            "SELECT id, lat, lon FROM geonames WHERE name=$1"));

    pqxx::result result= SHOW_EXCEPTIONS(sql->exec("Valassaaret"));
    BOOST_REQUIRE_EQUAL(result.size(), std::size_t(1));
    int id = result[0]["id"].as<int>();
    BOOST_CHECK_EQUAL(id, 632561);
}

BOOST_AUTO_TEST_CASE(test_prepared_sql_2, * utf::depends_on("get_connection_options"))
{
    db::PostgreSQLConnection conn(env::opts);
    BOOST_REQUIRE(conn.isConnected());
    auto sql = SHOW_EXCEPTIONS(
        std::make_shared<db::PostgreSQLConnection::PreparedSQL>(
            conn,
            "test2",
            "SELECT id, lat, lon FROM geonames WHERE name=$1 AND countries_iso2=$2"));

    const std::vector<std::string> params = { "Riga", "LV" };
    pqxx::result result= SHOW_EXCEPTIONS(sql->exec_p(params));
    BOOST_REQUIRE_EQUAL(result.size(), std::size_t(2));
    int id = result[0]["id"].as<int>();
    BOOST_CHECK_EQUAL(id, 456172);
}

BOOST_AUTO_TEST_CASE(test_prepared_sql_reuse_no_transaction, * utf::depends_on("get_connection_options"))
{
    int id;
    pqxx::result result;
    db::PostgreSQLConnection conn(env::opts);
    BOOST_REQUIRE(conn.isConnected());
    auto sql = SHOW_EXCEPTIONS(
        std::make_shared<db::PostgreSQLConnection::PreparedSQL>(
            conn,
            "test3",
            "SELECT id, lat, lon FROM geonames WHERE name=$1 ORDER BY id ASC"));

    result = SHOW_EXCEPTIONS(sql->exec("Riga"));
    BOOST_REQUIRE_EQUAL(result.size(), std::size_t(2));
    id = result[0]["id"].as<int>();
    BOOST_CHECK_EQUAL(id, -16003156);

    result = SHOW_EXCEPTIONS(sql->exec("Turku"));
    BOOST_REQUIRE_EQUAL(result.size(), std::size_t(3));
    id = result[0]["id"].as<int>();
    BOOST_CHECK_EQUAL(id, -105198);

    result = SHOW_EXCEPTIONS(sql->exec("Tampere"));
    BOOST_REQUIRE_EQUAL(result.size(), std::size_t(2));
    id = result[0]["id"].as<int>();
    BOOST_CHECK_EQUAL(id, 634963);
}

BOOST_AUTO_TEST_CASE(test_prepared_sql_reuse_in_transaction, * utf::depends_on("get_connection_options"))
{
    int id;
    pqxx::result result;
    db::PostgreSQLConnection conn(env::opts);
    BOOST_REQUIRE(conn.isConnected());
    auto sql = SHOW_EXCEPTIONS(
        std::make_shared<db::PostgreSQLConnection::PreparedSQL>(
            conn,
            "test4",
            "SELECT id, lat, lon FROM geonames WHERE name=$1 ORDER BY id ASC"));

    auto transaction = conn.transaction();

    result = SHOW_EXCEPTIONS(sql->exec("Riga"));
    BOOST_REQUIRE_EQUAL(result.size(), std::size_t(2));
    id = result[0]["id"].as<int>();
    BOOST_CHECK_EQUAL(id, -16003156);

    result = SHOW_EXCEPTIONS(sql->exec("Turku"));
    BOOST_REQUIRE_EQUAL(result.size(), std::size_t(3));
    id = result[0]["id"].as<int>();
    BOOST_CHECK_EQUAL(id, -105198);

    result = SHOW_EXCEPTIONS(sql->exec("Tampere"));
    BOOST_REQUIRE_EQUAL(result.size(), std::size_t(2));
    id = result[0]["id"].as<int>();
    BOOST_CHECK_EQUAL(id, 634963);
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

BOOST_AUTO_TEST_CASE(test_parametrized_sql_3, * utf::depends_on("get_connection_options"))
{
    db::PostgreSQLConnection conn(env::opts);
    BOOST_REQUIRE(conn.isConnected());
    auto transaction = conn.transaction();

    const std::vector<std::string> params = { "Riga", "LV" };

    pqxx::result result =
        SHOW_EXCEPTIONS(
            conn.exec_params_p(
                "SELECT id, lat, lon FROM geonames WHERE name=$1 AND countries_iso2=$2",
                params));
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

BOOST_AUTO_TEST_CASE(test_executeNonTransaction_after_parametrized_sql,
    * utf::depends_on("get_connection_options"))
{
    db::PostgreSQLConnection conn(env::opts);
    BOOST_REQUIRE(conn.isConnected());

    pqxx::result result;
    const std::vector<std::string> params = { "Riga", "LV" };

    do {
        auto transaction = conn.transaction();
        result =
            SHOW_EXCEPTIONS(
                conn.exec_params_p(
                    "SELECT id, lat, lon FROM geonames WHERE name=$1 AND countries_iso2=$2",
                    params));
        BOOST_REQUIRE_EQUAL(result.size(), std::size_t(2));
        int id = result[0]["id"].as<int>();
        BOOST_CHECK_EQUAL(id, 456172);
    } while (false);

    result = SHOW_EXCEPTIONS(conn.executeNonTransaction("SELECT COUNT(*) FROM geonames"));
    BOOST_REQUIRE_EQUAL(result.size(), std::size_t(1));
}

BOOST_AUTO_TEST_CASE(test_executeNonTransaction_after_using_prepared_sql,
    * utf::depends_on("get_connection_options"))
{
    db::PostgreSQLConnection conn(env::opts);
    BOOST_REQUIRE(conn.isConnected());

    pqxx::result result;

    do {
        db::PostgreSQLConnection conn(env::opts);
        BOOST_REQUIRE(conn.isConnected());
        auto sql = SHOW_EXCEPTIONS(
            std::make_shared<db::PostgreSQLConnection::PreparedSQL>(
                conn,
                "test",
                "SELECT id, lat, lon FROM geonames WHERE name=$1"));

        result= SHOW_EXCEPTIONS(sql->exec("Valassaaret"));
        BOOST_REQUIRE_EQUAL(result.size(), std::size_t(1));
        int id = result[0]["id"].as<int>();
        BOOST_CHECK_EQUAL(id, 632561);
    } while (false);

    result = SHOW_EXCEPTIONS(conn.executeNonTransaction("SELECT COUNT(*) FROM geonames"));
    BOOST_REQUIRE_EQUAL(result.size(), std::size_t(1));
}

BOOST_AUTO_TEST_CASE(test_executeNonTransaction_with_active_transaction,
    * utf::depends_on("get_connection_options"))
{
    db::PostgreSQLConnection conn(env::opts);
    BOOST_REQUIRE(conn.isConnected());

    pqxx::result result;

    {
        auto transaction = conn.transaction();
        result = SHOW_EXCEPTIONS(transaction->execute("SELECT id, lat, lon FROM geonames WHERE name='Helsinki'"));
        // Transaction is active: should fail
        BOOST_REQUIRE_THROW(
            result = conn.executeNonTransaction("SELECT COUNT(*) FROM geonames"),
            Fmi::Exception);
    }

    // Now the transaction has ended
    BOOST_REQUIRE_NO_THROW(
        result = conn.executeNonTransaction("SELECT COUNT(*) FROM geonames"));
}

BOOST_AUTO_TEST_CASE(postgresql_connection_pool_test, * utf::depends_on("get_connection_options"))
{
    db::PostgreSQLConnectionPool conn_pool(2, 5, env::opts);

    const auto do_test = [&conn_pool](const std::string& name) -> int {
        pqxx::result result;
        auto conn = conn_pool.get();
        if (not conn->isConnected())
            throw Fmi::Exception(BCP, "Database connection not open");
        auto sql = SHOW_EXCEPTIONS(
            std::make_shared<db::PostgreSQLConnection::PreparedSQL>(
                *conn.get(),
                "test",
                "SELECT id, lat, lon FROM geonames WHERE name=$1 ORDER BY id ASC"));

        auto transaction = conn->transaction();

        result = SHOW_EXCEPTIONS(sql->exec(name));
        if (result.size() < 1)
            throw Fmi::Exception(BCP, "No results found");
        int id = result[0]["id"].as<int>();
        return id;
    };

    int id1, id2, id3;
    std::future<int> f1 = std::async(std::launch::async, do_test, "Riga");
    std::future<int> f2 = std::async(std::launch::async, do_test, "Turku");
    std::future<int> f3 = std::async(std::launch::async, do_test, "Tampere");

    BOOST_REQUIRE_NO_THROW(id1 = f1.get());
    BOOST_REQUIRE_NO_THROW(id2 = f2.get());
    BOOST_REQUIRE_NO_THROW(id3 = f3.get());

    BOOST_CHECK_EQUAL(id1, -16003156);
    BOOST_CHECK_EQUAL(id2, -105198);
    BOOST_CHECK_EQUAL(id3, 634963);
}
BOOST_AUTO_TEST_CASE(test_immediate_reconnect_after_broken_connection,
    * utf::depends_on("get_connection_options"))
{
    // A connection broken while idle (here: backend terminated from another connection) is
    // replaced at once on the next query, without the 10 s wait between retries
    db::PostgreSQLConnection conn(env::opts);
    db::PostgreSQLConnection killer(env::opts);
    BOOST_REQUIRE(conn.isConnected());

    const auto backend_pid = [](db::PostgreSQLConnection& c)
    { return c.executeNonTransaction("SELECT pg_backend_pid() AS pid")[0]["pid"].as<int>(); };

    const int pid1 = backend_pid(conn);
    killer.executeNonTransaction("SELECT pg_terminate_backend(" + std::to_string(pid1) + ")");

    const auto start = std::chrono::steady_clock::now();
    int pid2 = 0;
    BOOST_REQUIRE_NO_THROW(pid2 = backend_pid(conn));
    const double elapsed =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

    BOOST_CHECK_NE(pid1, pid2);
    BOOST_CHECK_LT(elapsed, 5.0);
}

BOOST_AUTO_TEST_CASE(test_broken_connection_in_transaction,
    * utf::depends_on("get_connection_options"))
{
    // A connection broken during a transaction aborts the transaction (no retry of the single
    // statement on a new connection); the connection itself is usable again afterwards
    db::PostgreSQLConnection conn(env::opts);
    db::PostgreSQLConnection killer(env::opts);
    BOOST_REQUIRE(conn.isConnected());

    const auto backend_pid = [](db::PostgreSQLConnection& c)
    { return c.executeNonTransaction("SELECT pg_backend_pid() AS pid")[0]["pid"].as<int>(); };

    const int pid1 = backend_pid(conn);
    const auto start = std::chrono::steady_clock::now();
    {
        auto transaction = conn.transaction();
        transaction->execute("CREATE TEMP TABLE broken_txn_test(i int)");
        killer.executeNonTransaction("SELECT pg_terminate_backend(" + std::to_string(pid1) + ")");
        BOOST_CHECK_THROW(transaction->execute("SELECT count(*) FROM broken_txn_test"),
                          Fmi::Exception);
        BOOST_CHECK_THROW(transaction->commit(), Fmi::Exception);
    }
    const double elapsed =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    BOOST_CHECK_LT(elapsed, 5.0);

    int pid2 = 0;
    BOOST_REQUIRE_NO_THROW(pid2 = backend_pid(conn));
    BOOST_CHECK_NE(pid1, pid2);
}
