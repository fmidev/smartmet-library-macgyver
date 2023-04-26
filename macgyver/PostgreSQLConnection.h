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

    template <typename... Args>
    pqxx::result executePrepared(const std::string& name, Args &&... args) const
    {
      try
      {
        auto transaction_ptr = conn.get_transaction_impl();
        return transaction_ptr->exec_params(name, args...);
      }
      catch (...)
      {
        throw Fmi::Exception::Trace(BCP, "Operation failed!");
      }
    }

    template <typename... Args>
    pqxx::result executePrepared_N(const std::string& name, unsigned n, Args... args) const
    {
      try
      {
        auto transaction_ptr = conn.get_transaction_impl();
        return transaction_ptr->exec_params(name, args...);
      }
      catch (...)
      {
        throw Fmi::Exception::Trace(BCP, "Operation failed!");
      }
    }

    void commit();
    void rollback();

   private:
    const PostgreSQLConnection& conn;
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

  void prepare(const std::string& name, const std::string& theSQLStatement);

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

}  // namespace Database
}  // namespace Fmi

// ======================================================================
