// ======================================================================
/*!
 * \brief Interface of class Fmi::Connection
 */
// ======================================================================

#pragma once

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <list>
#include <pqxx/pqxx>
#include <string>

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
  class Transaction final : public virtual boost::noncopyable
  {
   public:
    Transaction(PostgreSQLConnection& conn);
    ~Transaction();
    pqxx::result execute(const std::string& theSQLStatement) const;
    void commit();
    void rollback();

   private:
    PostgreSQLConnection& conn;
  };

  ~PostgreSQLConnection() { close(); }
  PostgreSQLConnection(bool theDebug = false) : itsDebug(theDebug), itsCollate(false) {}
  PostgreSQLConnection(const PostgreSQLConnectionOptions& theConnectionOptions);

  bool open(const PostgreSQLConnectionOptions& theConnectionOptions);
  bool reopen();

  void close();
  bool isConnected() const { return itsConnection->is_open(); }
  void setClientEncoding(const std::string& theEncoding) const;
  void setDebug(bool debug) { itsDebug = debug; }
  pqxx::result executeNonTransaction(const std::string& theSQLStatement) const;

  /**
   *   @brief Execute SQL statement in current transaction or call executeNonTransaction if no
   *          transaction is active
   */
  pqxx::result execute(const std::string& theSQLStatement) const;
  void cancel();

  bool collateSupported() { return itsCollate; }
  std::string quote(const std::string& theString) const;
  const std::map<unsigned int, std::string>& dataTypes() const { return itsDataTypes; }

  std::shared_ptr<Transaction> transaction() { return std::make_shared<Transaction>(*this); }

 private:
  boost::shared_ptr<pqxx::connection> itsConnection;  // PostgreSQL connecton
  boost::shared_ptr<pqxx::work> itsTransaction;       // PostgreSQL transaction
  bool itsDebug = false;
  bool itsCollate = false;
  PostgreSQLConnectionOptions itsConnectionOptions;
  std::map<unsigned int, std::string> itsDataTypes;
};  // class PostgreSQLConnection

}  // namespace Database
}  // namespace Fmi

// ======================================================================
