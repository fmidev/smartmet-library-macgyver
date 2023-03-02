#include "PostgreSQLConnection.h"
#include "Exception.h"
#include "TypeName.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/variant.hpp>
#include <fmt/format.h>
#include <cassert>
#include <iostream>
#include <sstream>
#include <vector>

namespace ba = boost::algorithm;

namespace Fmi
{
namespace Database
{
namespace
{
using uint_member_ptr = unsigned int PostgreSQLConnectionOptions::*;
using string_member_ptr = std::string PostgreSQLConnectionOptions::*;

struct Ignore
{
} ignore;

const std::map<std::string, boost::variant<Ignore, uint_member_ptr, string_member_ptr> > field_def =
    {{"host", &PostgreSQLConnectionOptions::host},
     {"dbname", &PostgreSQLConnectionOptions::database},
     {"port", &PostgreSQLConnectionOptions::port},
     {"user", &PostgreSQLConnectionOptions::username},
     {"password", &PostgreSQLConnectionOptions::password},
     {"client_encoding", &PostgreSQLConnectionOptions::encoding},
     {"connect_timeout", &PostgreSQLConnectionOptions::connect_timeout}
     // FIXME: add parameters we ignore
     ,
     {"options", ignore}};
}  // namespace

// ----------------------------------------------------------------------

std::atomic<bool> PostgreSQLConnection::shuttingDown(false);
std::atomic<bool> PostgreSQLConnection::reconnectDisabled(false);

PostgreSQLConnectionOptions::PostgreSQLConnectionOptions(const std::string& conn_str)
{
  try
  {
    std::vector<std::string> parts;
    ba::split(parts, conn_str, ba::is_any_of(" \t"), ba::token_compress_on);
    for (const std::string& part : parts)
    {
      const std::size_t p = part.find('=');
      if (p == std::string::npos)
        throw Fmi::Exception(BCP, "Unrecognized part '" + part + "'");

      const std::string name = part.substr(0, p);
      const std::string value = part.substr(p + 1);
      const auto it = field_def.find(name);
      if (it == field_def.end())
      {
        throw Fmi::Exception(BCP, "Unrecognized field '" + part + "'");
      }

      switch (it->second.which())
      {
        case 0:
          // std::cout << METHOD_NAME << ": field '" << part << "' ignored" << std::endl;
          break;
        case 1:
          this->*boost::get<uint_member_ptr>(it->second) = boost::lexical_cast<unsigned int>(value);
          break;
        case 2:
          this->*boost::get<string_member_ptr>(it->second) = value;
          break;

        default:  // Not supposed to be here
          assert("Not supposed to be here");
      }
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Failed to parse connection string '" + conn_str + "'");
  }
}

PostgreSQLConnectionOptions::operator std::string() const
{
  std::ostringstream ss;

  // clang-format off
  ss << "host="      << host
     << " dbname="   << database
     << " port="    << port
     << " user="     << username
     << " password=" << password
#if 0
     << " client_encoding=" << encoding
#endif
       ;
  // clang-format on

  if (connect_timeout > 0)
    ss << " connect_timeout=" << connect_timeout;

  return ss.str();
}

// ----------------------------------------------------------------------

class PostgreSQLConnection::Impl
{
 private:
  mutable std::shared_ptr<pqxx::connection> itsConnection;  // PostgreSQL connecton
  mutable std::shared_ptr<pqxx::work> itsTransaction;       // PostgreSQL transaction
  bool itsDebug = false;
  bool itsCollate = false;
  std::atomic<bool> itsCanceled;
  PostgreSQLConnectionOptions itsConnectionOptions;
  mutable std::map<unsigned int, std::string> itsDataTypes;
  mutable boost::condition_variable cond;
  mutable boost::mutex m;

  mutable bool last_failed = false;

 public:
  ~Impl() { close(); }

  Impl(bool debug) : itsDebug(debug) {}

  Impl(const PostgreSQLConnectionOptions& theConnectionOptions)
      : itsDebug(theConnectionOptions.debug)
      , itsCanceled(false)
      , itsConnectionOptions(theConnectionOptions)
  {
    open(itsConnectionOptions);
  }

  bool open(const PostgreSQLConnectionOptions& theConnectionOptions)
  {
    itsCanceled.store(false);
    itsConnectionOptions = theConnectionOptions;
    last_failed = false;
    return bool(reopen());
  }

  bool isTransaction() const { return !!itsTransaction; }

  void startTransaction()
  {
     auto conn = check_connection();
     if (conn)
     {
        itsTransaction = std::make_shared<pqxx::work>(*conn);
     }
     else
     {
        Fmi::Exception(BCP, METHOD_NAME + ": cannot start transaction (not connected)");
     }
  }

  void endTransaction()
  {
    if (itsTransaction)
      itsTransaction.reset();
    else
      throw Fmi::Exception(BCP, "Not in transaction");
  }

  void commitTransaction()
  {
    if (itsTransaction)
      itsTransaction->commit();
    else
      throw Fmi::Exception(BCP, "Not in transaction");
  }

  void exec(const std::string& sql) { itsTransaction->exec(sql); }

  void cancel()
  {
    if (itsConnection)
    {
      itsCanceled.store(true);
      itsConnection->cancel_query();
      boost::unique_lock<boost::mutex> lock(m);
      cond.notify_one();
    }
  }

  void close() const
  {
    itsTransaction.reset();
    itsConnection.reset();
  }

  bool isConnected() const { return itsConnection->is_open(); }
  bool collateSupported() const { return itsCollate; }
  const std::map<unsigned int, std::string>& dataTypes() const { return itsDataTypes; }

  bool isDebug() const { return itsDebug; }
  void setDebug(bool debug) { itsDebug = debug; }

  std::shared_ptr<pqxx::connection> check_connection() const
  {
    if (itsConnection && isConnected())
    {
      return itsConnection;
    }
    else
    {
      if (reconnectDisabled.load() || itsCanceled.load())
      {
        throw Fmi::Exception(BCP, METHOD_NAME + ": not connected and reconnecting is disabled or connection canceled");
      }
      else
      {
        return reopen();
      }
    }
  }

  void setClientEncoding(const std::string& theEncoding)
  {
    try
    {
      auto conn = check_connection();
      if (conn)
        conn->set_client_encoding(theEncoding);

      itsConnectionOptions.encoding = theEncoding;
    }
    catch (...)
    {
      throw Fmi::Exception(BCP, "set_client_encoding failed").addParameter("encoding", theEncoding);
    }
  }

  std::string quote(const std::string& theString) const
  {
    try
    {
      auto conn = check_connection();
      if (conn)
        return conn->quote(theString);
      throw Fmi::Exception(BCP, "Locus: Attempting to quote string without database connection");
    }
    catch (...)
    {
      throw Fmi::Exception::Trace(BCP, "Operation failed!");
    }
  }

  std::shared_ptr<pqxx::connection> reopen() const
  {
    try
    {
      close();

      const std::string conn_str = itsConnectionOptions;

      boost::optional<std::string> error_message;

      // Retry connections automatically. Especially useful after boots if the database is in the
      // same server.
      const int max_retries = last_failed ? 1 : 10;

      for (int retries = max_retries; retries > 0; --retries)
      {
        // Ignore connection requests if shutting down application
        if (shuttingDown.load())
        {
          throw Fmi::Exception(BCP, METHOD_NAME + ": connecting to database disabled due to shutdown");
        }

        try
        {
          itsConnection = std::make_shared<pqxx::connection>(conn_str);
          /*
          if(PostgreSQL > 9.1)
          itsCollate = true;
          pqxx::result res = executeNonTransaction("SELECT version()");
        */
        }
        catch (const pqxx::broken_connection& e)
        {
          last_failed = true;
          if (reconnectDisabled.load())
          {
            // Should not try to reconnect (rethrown current exception)
            throw;
          }
          else
          {
            if (retries > 0)
            {
              std::string msg = e.what();
              boost::algorithm::replace_all(msg, "\n", " ");
              std::cerr << fmt::format("Warning: {} retries left. PG message: {}\n", retries, msg);
              boost::unique_lock<boost::mutex> lock(m);
              if (! shuttingDown.load())
              {
                cond.wait_for(lock, boost::chrono::seconds(10));
              }
            }
            else
              error_message = e.what();
          }
        }
        catch (const std::exception& e)
        {
          error_message = e.what();
          break;
        }

        if (error_message)
          throw Fmi::Exception(BCP,
                               "Failed to connect to " + itsConnectionOptions.username + "@" +
                                   itsConnectionOptions.database + ":" +
                                   std::to_string(itsConnectionOptions.port) + " : " +
                                   *error_message);

        // Store info of data types
        if (itsConnection)
        {
          pqxx::result result_set;
          const std::string sql = "select typname,oid from pg_type";
          try
          {
            // Do not use executeNonTransaction here, as we do not want to call reopen() recursively
            pqxx::nontransaction nitsTransaction(*itsConnection);
            result_set = nitsTransaction.exec(sql);
          }
          catch (const std::exception& e)
          {
            std::cout << "Failed to execute '" << sql << "':" << e.what() << std::endl;
            continue;
          }

          for (auto row : result_set)
          {
            auto datatype = row[0].as<std::string>();
            unsigned int oid = row[1].as<unsigned int>();
            itsDataTypes.insert(std::make_pair(oid, datatype));
          }

          if (itsConnection->is_open())
          {
            last_failed = false;
            return itsConnection;
          }
        }
      }

      return std::shared_ptr<pqxx::connection>();
    }
    catch (...)
    {
      throw Fmi::Exception::Trace(BCP, "Operation failed!");
    }
  }

  pqxx::result executeNonTransaction(const std::string& theSQLStatement) const
  {
    // FIXME: should we fail if transaction is active?
    try
    {
      if (itsDebug)
        std::cout << "SQL: " << theSQLStatement << std::endl;

      auto conn = check_connection();
      if (! conn)
      {
        throw Fmi::Exception(BCP,
                             "Execution of SQL statement failed: not connected");
      }

      try
      {
        pqxx::nontransaction nitsTransaction(*conn);
        return nitsTransaction.exec(theSQLStatement);
      }
      catch (const std::exception& e)
      {
        // Try reopening the connection only once not to flood the network
        if (conn->is_open())
        {
          throw Fmi::Exception(BCP,
                               std::string("Execution of SQL statement failed: ").append(e.what()));
        }
        else
        {
          conn = reopen();
          if (conn)
          {
            try
            {
              pqxx::nontransaction nitsTransaction(*conn);
              return nitsTransaction.exec(theSQLStatement);
            }
            catch (const std::exception& e)
            {
              throw Fmi::Exception(BCP,
                                   std::string("Execution of SQL statement failed: ").append(e.what()));
            }
          }
          else
          {
              throw Fmi::Exception(BCP,
                                   "Execution of SQL statement failed: not connected");
          }
        }
      }
    }
    catch (...)
    {
      throw Fmi::Exception::Trace(BCP, "Operation failed!");
    }
  }

  pqxx::result execute(const std::string& theSQLStatement) const
  {
    try
    {
      if (itsTransaction)
      {
        // Cannot call check_connection here as possible reopen() would discard transaction
        return itsTransaction->exec(theSQLStatement);
      }

      return executeNonTransaction(theSQLStatement);
    }
    catch (...)
    {
      throw Fmi::Exception::Trace(BCP, "Operation failed!");
    }
  }
};

// ----------------------------------------------------------------------

PostgreSQLConnection::PostgreSQLConnection(const PostgreSQLConnectionOptions& theConnectionOptions)
    : impl(new Impl(theConnectionOptions))
{
}

bool PostgreSQLConnection::open(const PostgreSQLConnectionOptions& theConnectionOptions)
{
  try
  {
    return bool(impl->open(theConnectionOptions));
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

bool PostgreSQLConnection::reopen() const
{
  try
  {
    return bool(impl->reopen());
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

void PostgreSQLConnection::close() const
{
  try
  {
    impl->close();
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

std::string PostgreSQLConnection::quote(const std::string& theString) const
{
  try
  {
    return impl->quote(theString);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

pqxx::result PostgreSQLConnection::executeNonTransaction(const std::string& theSQLStatement) const
{
  try
  {
    return impl->executeNonTransaction(theSQLStatement);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

pqxx::result PostgreSQLConnection::execute(const std::string& theSQLStatement) const
{
  try
  {
    return impl->execute(theSQLStatement);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

void PostgreSQLConnection::cancel()
{
  try
  {
    impl->cancel();
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Transaction cancellation failed!");
  }
}

void PostgreSQLConnection::setClientEncoding(const std::string& theEncoding)
{
  try
  {
    impl->setClientEncoding(theEncoding);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

PostgreSQLConnection::PostgreSQLConnection(bool theDebug) : impl(new Impl(theDebug)) {}

bool PostgreSQLConnection::isConnected() const
{
  return impl->isConnected();
}
void PostgreSQLConnection::setDebug(bool debug)
{
  impl->setDebug(debug);
}

bool PostgreSQLConnection::isDebug() const
{
  return impl->isDebug();
}

bool PostgreSQLConnection::collateSupported() const
{
  return impl->collateSupported();
}

const std::map<unsigned int, std::string>& PostgreSQLConnection::dataTypes() const
{
  return impl->dataTypes();
}

std::shared_ptr<PostgreSQLConnection::Transaction> PostgreSQLConnection::transaction() const
{
  return std::make_shared<PostgreSQLConnection::Transaction>(*this);
}

bool PostgreSQLConnection::isTransaction() const
{
  return impl->isTransaction();
}

void PostgreSQLConnection::startTransaction() const
{
  impl->startTransaction();
}

void PostgreSQLConnection::endTransaction() const
{
  impl->endTransaction();
}

void PostgreSQLConnection::commitTransaction() const
{
  impl->commitTransaction();
}

PostgreSQLConnection::~PostgreSQLConnection() = default;

// ----------------------------------------------------------------------

PostgreSQLConnection::Transaction::Transaction(const PostgreSQLConnection& theConnection)
    : conn(theConnection)
{
  try
  {
    if (conn.isTransaction())
      throw Fmi::Exception(BCP, "Recursive transactions are not supported");

    conn.startTransaction();
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

PostgreSQLConnection::Transaction::~Transaction()
{
  conn.endTransaction();
}

pqxx::result PostgreSQLConnection::Transaction::execute(const std::string& theSQLStatement) const
{
  try
  {
    if (conn.isTransaction())
    {
      if (conn.isDebug())
      {
        std::cout << "SQL: " << theSQLStatement << std::endl;
      }
      return conn.execute(theSQLStatement);
    }

    throw Fmi::Exception(BCP, "[Logic error] Called after transaction commit or rollback");
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

void PostgreSQLConnection::Transaction::commit()
{
  try
  {
    if (conn.isTransaction())
    {
      try
      {
        conn.commitTransaction();
        conn.endTransaction();
      }
      catch (const std::exception& e)
      {
        // If we get here, Xaction has been rolled back
        conn.endTransaction();
        throw Fmi::Exception(BCP, std::string("Commiting transaction failed: ").append(e.what()));
      }
    }
    else
    {
      throw Fmi::Exception(BCP,
                           "[Logic error] No transaction to commit"
                           " (already commited or rolled back)");
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

void PostgreSQLConnection::Transaction::rollback()
{
  try
  {
    if (conn.isTransaction())
    {
      conn.endTransaction();
    }
    else
    {
      throw Fmi::Exception(BCP,
                           "[Logic error] No transaction to roll back"
                           " (already commited or rolled back)");
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace Database
}  // namespace Fmi

// ======================================================================
