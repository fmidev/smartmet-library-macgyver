// ======================================================================
/*!
 * \brief Interface of class Fmi::Connection
 */
// ======================================================================

#pragma once

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
};

class PostgreSQLConnection
{
 public:
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
  void startTransaction();
  pqxx::result executeTransaction(const std::string& theSQLStatement) const;
  void commitTransaction();
  void cancel();

  bool collateSupported() { return itsCollate; }
  std::string quote(const std::string& theString) const;
  const std::map<unsigned int, std::string>& dataTypes() const { return itsDataTypes; }

 private:
  boost::shared_ptr<pqxx::connection> itsConnection;  // PostgreSQL connecton
  boost::shared_ptr<pqxx::work> itsTransaction;       // PostgreSQL transaction
  bool itsDebug;
  bool itsCollate;
  PostgreSQLConnectionOptions itsConnectionOptions;
  std::map<unsigned int, std::string> itsDataTypes;
};  // class PostgreSQLConnection

}  // namespace Database
}  // namespace Fmi

// ======================================================================
