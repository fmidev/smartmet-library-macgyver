// ======================================================================
/*!
 * \brief Interface of class Fmi::Connection
 */
// ======================================================================

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <pqxx/pqxx>
#include <string>
#include "Exception.h"
#include "TypeTraits.h"

namespace Fmi
{
namespace Database
{
struct PostgreSQLConnectionOptions
{
  std::string host;
  unsigned int port = 5432;
  std::string database;
  std::string username;
  std::string password;
  std::string encoding = "UTF8";
  unsigned int connect_timeout = 0;
  bool debug = false;

  PostgreSQLConnectionOptions() = default;
  PostgreSQLConnectionOptions(const std::string& conn_str);

  operator std::string() const;
};

class PostgreSQLConnection
{
 public:
  class Transaction final
  {
  public:
    ~Transaction();
    Transaction(const PostgreSQLConnection& theConnection);

    Transaction() = delete;
    Transaction(const Transaction& other) = delete;
    Transaction& operator=(const Transaction& other) = delete;
    Transaction(Transaction&& other) = delete;
    Transaction& operator=(Transaction&& other) = delete;

    pqxx::result execute(const std::string& theSQLStatement) const;

    void commit();
    void rollback();

   private:
    const PostgreSQLConnection& conn;
  };

  class PreparedSQL final
  {
      const PostgreSQLConnection& conn;
      const std::string name;
      const std::string sql;
   public:
      PreparedSQL(const PostgreSQLConnection& theConnection, const std::string& name, const std::string& sql);
      ~PreparedSQL();

      /**
       *   @brief execute prepared SQL statement with parameters provided in a container
       *         (eg. std::vector)
       *
       *   @param container container with parameters
       *   @param requested_size requested row count in the response
       *                         (default -1, negative value means no limits)
       */
      template <typename Container>
      typename std::enable_if<is_iterable<Container>::value, pqxx::result>::type
      exec_p(const Container& container, int requested_size = -1);

      template <typename... Args>
      pqxx::result exec(Args... args)
      {
        try
        {
          auto transaction_ptr = conn.get_transaction_impl();
          return transaction_ptr->exec_prepared(name, args...);
        }
        catch (...)
        {
          throw Fmi::Exception::Trace(BCP, "Operation failed!");
        }
      }

      template <typename... Args>
      pqxx::result exec_n(std::size_t num_rows, Args... args)
      {
        try
        {
          auto transaction_ptr = conn.get_transaction_impl();
          return transaction_ptr->exec_prepared_n(num_rows, name, args...);
        }
        catch (...)
        {
          throw Fmi::Exception::Trace(BCP, "Operation failed!");
        }
      }
  };

  ~PostgreSQLConnection();
  PostgreSQLConnection(bool theDebug = false);
  PostgreSQLConnection(const PostgreSQLConnectionOptions& theConnectionOptions);

  bool open(const PostgreSQLConnectionOptions& theConnectionOptions);
  bool reopen() const;

  void close() const;
  bool isConnected() const;
  void setClientEncoding(const std::string& theEncoding);
  bool isDebug() const;
  void setDebug(bool debug);
  pqxx::result executeNonTransaction(const std::string& theSQLStatement) const;

  /**
   *   @brief Execute SQL statement in current transaction or call executeNonTransaction if no
   *          transaction is active
   */
  pqxx::result execute(const std::string& theSQLStatement) const;
  void cancel();

  template <typename... Args>
  pqxx::result exec_params(const std::string& theSQLStatement, Args... args)
  {
     auto transaction_ptr = get_transaction_impl();
     return transaction_ptr->exec_params(theSQLStatement, args...);
  }

  template <typename... Args>
  pqxx::result exec_params_n(std::size_t num_rows, const std::string& theSQLStatement, Args... args)
  {
     auto transaction_ptr = get_transaction_impl();
     return transaction_ptr->exec_params_n(num_rows, theSQLStatement, args...);
  }

  /**
   *   @brief execute prepared SQL statement with parameters provided in a container
   *         (eg. std::vector)
   *
   *   @param container container with parameters
   *   @param requested_size requested row count in the response
   *                         (default -1, negative value means no limits)
   */
  template <typename Container>
  typename std::enable_if<is_iterable<Container>::value, pqxx::result>::type
  exec_params_p(
      const std::string& theSQLStatement,
      const Container& container,
      int requested_size =  -1);

  bool collateSupported() const;
  std::string quote(const std::string& theString) const;
  const std::map<unsigned int, std::string>& dataTypes() const;

  std::shared_ptr<Transaction> transaction() const;

  /**
   *   @brief Notifies all PostgreSQL objects that application shutdown is in progress
   *
   *   Note that this call cannot be undone without restarting application
   */
  static void shutdownAll() { shuttingDown.store(true); }

  /**
   *   @brief Disable reconnect attempts for all PostgreSQL objects (for use in tests)
   */
    static void disableReconnect() { reconnectDisabled.store(true); }

 private:
  bool isTransaction() const;
  void startTransaction() const;
  void endTransaction() const;
  void commitTransaction() const;

  std::shared_ptr<pqxx::transaction_base> get_transaction_impl() const;

  pqxx::work get_work() const;

  class Impl;
  std::unique_ptr<Impl> impl;

  static std::atomic<bool> shuttingDown;
  static std::atomic<bool> reconnectDisabled;

};  // class PostgreSQLConnection


namespace detail
{

#if PQXX_VERSION_MAJOR < 7

template <typename Container>
auto make_params(const Container& container) -> decltype(pqxx::prepare::make_dynamic_params(container))
{
  return pqxx::prepare::make_dynamic_params(container);
}

#else // PQXX_VERSION >= 7

template <typename Container>
  auto make_params(const Container& container) -> pqxx::params
{
  pqxx::params params;
  params.append_multi(container);
  return params;
}

#endif

} // namespace detail

template <typename Container>
typename std::enable_if<is_iterable<Container>::value, pqxx::result>::type
PostgreSQLConnection::PreparedSQL::exec_p(const Container& container, int requested_size)
{
  try
  {
    auto transaction_ptr = conn.get_transaction_impl();
    const auto params = detail::make_params(container);
    if (requested_size < 0) {
      return transaction_ptr->exec_prepared(name, params);
     }
    return transaction_ptr->exec_prepared_n(requested_size, name, params);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

template <typename Container>
typename std::enable_if<is_iterable<Container>::value, pqxx::result>::type
PostgreSQLConnection::exec_params_p(
    const std::string& theSQLStatement,
    const Container& container,
    int requested_size)
{
  // No easy way to use the same method name as for Args... as we need to distinguish from
  // cases when std::string<something> is provided as the only argument
  try
  {
    auto transaction_ptr = get_transaction_impl();
    const auto params = detail::make_params(container);
    if (requested_size < 0) {
      return transaction_ptr->exec_params(theSQLStatement, params);
    }
    return transaction_ptr->exec_params_n(requested_size, theSQLStatement, params);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace Database
}  // namespace Fmi

// ======================================================================
