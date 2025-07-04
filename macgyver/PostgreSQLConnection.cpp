#include "PostgreSQLConnection.h"
#include "AsyncTask.h"
#include "Exception.h"
#include "NumericCast.h"
#include "StringConversion.h"
#include "TypeName.h"
#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>
#include <fmt/format.h>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <optional>
#include <sstream>
#include <variant>
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

constexpr std::monostate ignore;

const std::map<std::string, std::variant<std::monostate, uint_member_ptr, string_member_ptr> >
    field_def = {{"host", &PostgreSQLConnectionOptions::host},
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

      switch (it->second.index())
      {
        case 0:
          // std::cout << METHOD_NAME << ": field '" << part << "' ignored" << std::endl;
          break;
        case 1:
          this->*std::get<uint_member_ptr>(it->second) =
              Fmi::numeric_cast<unsigned int>(Fmi::stoul(value));
          break;
        case 2:
          this->*std::get<string_member_ptr>(it->second) = value;
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

  std::map<std::string, std::string> prepared_sqls;

 public:
  ~Impl() { close(); }

  Impl(bool debug) : itsDebug(debug) {}

  Impl(const PostgreSQLConnectionOptions& theConnectionOptions)
      : itsDebug(theConnectionOptions.debug),
        itsCanceled(false),
        itsConnectionOptions(theConnectionOptions)
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
      throw Fmi::Exception(BCP, METHOD_NAME + ": cannot start transaction (not connected)");
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

  /**
   *   @brief return current connection if it is OK or empty shared_ptr otherwise
   */
  std::shared_ptr<pqxx::connection> get_connection() const
  {
    if (itsConnection && isConnected())
      return itsConnection;

    return {};
  }

  std::shared_ptr<pqxx::connection> check_connection() const
  {
    if (itsConnection && isConnected())
      return itsConnection;

    if (reconnectDisabled.load() || itsCanceled.load())
    {
      throw Fmi::Exception(
          BCP, METHOD_NAME + ": not connected and reconnecting is disabled or connection canceled");
    }

    return reopen();
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

      AsyncTask::interruption_point();

      const std::string conn_str = itsConnectionOptions;

      std::optional<std::string> error_message;

      // Retry connections automatically. Especially useful after boots if the database is in the
      // same server.
      const int max_retries = last_failed ? 1 : 10;

      for (int retries = max_retries; retries > 0; --retries)
      {
        // Ignore connection requests if shutting down application
        if (shuttingDown.load())
        {
          throw Fmi::Exception(BCP,
                               METHOD_NAME + ": connecting to database disabled due to shutdown");
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

          if (retries > 1)
          {
            std::string msg = e.what();
            boost::algorithm::replace_all(msg, "\n", " ");
            std::cerr << fmt::format("Warning: {} retries left. PG message: {}\n", retries, msg);
            boost::unique_lock<boost::mutex> lock(m);
            if (!shuttingDown.load())
            {
              cond.wait_for(lock, boost::chrono::seconds(10));
            }
          }
          else
            error_message = e.what();
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
            const auto datatype = row[0].as<std::string>();
            const auto oid = row[1].as<unsigned int>();
            itsDataTypes.insert(std::make_pair(oid, datatype));
          }

          boost::unique_lock<boost::mutex> lock(m);
          for (const auto& item : prepared_sqls)
          {
            try
            {
              itsConnection->prepare(item.first, item.second);
            }
            catch (const std::exception& e)
            {
              std::cout << "Error restoring prepared SQL statement: name=" << item.first << " sql='"
                        << item.second << "': " << e.what() << std::endl;
            }
          }

          if (itsConnection->is_open())
          {
            last_failed = false;
            return itsConnection;
          }
        }
      }

      return {};
    }
    catch (...)
    {
      throw Fmi::Exception::Trace(BCP, "Operation failed!");
    }
  }

  void checkSlowQuery(const std::string& theSQLStatement,
                      const std::chrono::time_point<std::chrono::high_resolution_clock>& start,
                      const std::chrono::time_point<std::chrono::high_resolution_clock>& end) const
  {
    auto limit = itsConnectionOptions.slow_query_limit;

    if (limit > 0)
    {
      const auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
      if (duration.count() >= limit)
      {
        auto sql = theSQLStatement;
        std::replace(sql.begin(), sql.end(), '\n', ' ');
        std::cerr << fmt::format("{} Slow {}:{} query took {} seconds, limit is {}. SQL: {}\n",
                                 Fmi::to_simple_string(Fmi::SecondClock::local_time()),
                                 itsConnectionOptions.host,
                                 itsConnectionOptions.database,
                                 duration.count(),
                                 limit,
                                 sql);
      }
    }
  }

  pqxx::result executeNonTransaction(const std::string& theSQLStatement) const
  {
    // FIXME: should we fail if transaction is active?
    try
    {
      AsyncTask::interruption_point();

      if (itsDebug)
        std::cout << "SQL: " << theSQLStatement << std::endl;

      auto conn = check_connection();
      if (!conn)
      {
        throw Fmi::Exception(BCP, "Execution of SQL statement failed: not connected");
      }

      try
      {
        pqxx::nontransaction nitsTransaction(*conn);

        const auto start = std::chrono::high_resolution_clock::now();
        auto result = nitsTransaction.exec(theSQLStatement);
        const auto end = std::chrono::high_resolution_clock::now();
        checkSlowQuery(theSQLStatement, start, end);
        return result;
      }
      catch (const std::exception& e)
      {
        // Try reopening the connection only once not to flood the network
        if (conn->is_open())
        {
          throw Fmi::Exception(BCP, "Execution of SQL statement failed").addDetail(e.what());
        }
        else
        {
          conn = reopen();
          if (conn)
          {
            try
            {
              pqxx::nontransaction nitsTransaction(*conn);
              const auto start = std::chrono::high_resolution_clock::now();
              auto result = nitsTransaction.exec(theSQLStatement);
              const auto end = std::chrono::high_resolution_clock::now();
              checkSlowQuery(theSQLStatement, start, end);
              return result;
            }
            catch (const std::exception& e2)
            {
              throw Fmi::Exception(BCP, "Execution of SQL statement failed").addDetail(e2.what());
            }
          }
          else
          {
            throw Fmi::Exception(BCP, "Execution of SQL statement failed: not connected");
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
        AsyncTask::interruption_point();
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

  std::shared_ptr<pqxx::transaction_base> get_transaction_impl()
  {
    try
    {
      AsyncTask::interruption_point();
      if (itsTransaction)
      {
        return itsTransaction;
      }
      else
      {
        auto conn = check_connection();
        return std::shared_ptr<pqxx::transaction_base>(new pqxx::nontransaction(*conn));
      }
    }
    catch (...)
    {
      throw Fmi::Exception::Trace(BCP, "Operation failed!");
    }
  }

  void register_prepared_sql(const std::string& name, const std::string& sql)
  {
    boost::unique_lock<boost::mutex> lock(m);
    prepared_sqls[name] = sql;
  }

  void unregister_prepared_sql(const std::string& name)
  {
    boost::unique_lock<boost::mutex> lock(m);
    prepared_sqls.erase(name);
  }

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;
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

PostgreSQLConnection::PreparedSQL::Ptr PostgreSQLConnection::prepare(
    const std::string& name, const std::string& theSQLStatement) const
{
  return std::make_shared<PreparedSQL>(*this, name, theSQLStatement);
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

std::shared_ptr<pqxx::transaction_base> PostgreSQLConnection::get_transaction_impl() const
{
  return impl->get_transaction_impl();
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
  try
  {
    conn.endTransaction();
  }
  catch (...)
  {
  }
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
        throw Fmi::Exception(BCP, "Commiting transaction failed").addDetail(e.what());
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

PostgreSQLConnection::PreparedSQL::PreparedSQL(const PostgreSQLConnection& theConnection,
                                               const std::string& name,
                                               const std::string& theSQLStatement)

    : conn(theConnection), name(name), sql(theSQLStatement)
{
  try
  {
    auto c = conn.impl->check_connection();
    if (!c)
    {
      throw Fmi::Exception(BCP, "Execution of SQL statement failed: not connected");
    }

    c->prepare(name, sql);

    // FIXME: report conflicts (repeated sama name). Additionally one could ignore call
    //        if the new SQL statement is identical to the previous one
    conn.impl->register_prepared_sql(name, theSQLStatement);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

PostgreSQLConnection::PreparedSQL::~PreparedSQL()
{
  try
  {
    // We do not to unprepare if connection is lost and not restored
    auto c = conn.impl->get_connection();
    conn.impl->unregister_prepared_sql(name);
    if (c)
      c->unprepare(name);
  }
  catch (...)
  {
    // We are not interested about errors when unpreparing SQL (eg. not found)
  }
}

}  // namespace Database
}  // namespace Fmi

// ======================================================================
