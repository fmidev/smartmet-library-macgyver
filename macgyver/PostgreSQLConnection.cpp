#include "PostgreSQLConnection.h"
#include "Exception.h"
#include "TypeName.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/variant.hpp>
#include <fmt/format.h>
#include <cassert>
#include <iostream>
#include <sstream>
#include <thread>
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
  std::shared_ptr<pqxx::work> itsTransaction;               // PostgreSQL transaction
  bool itsDebug = false;
  bool itsCollate = false;
  PostgreSQLConnectionOptions itsConnectionOptions;
  mutable std::map<unsigned int, std::string> itsDataTypes;

 public:
  ~Impl() { close(); }

  Impl(bool debug) : itsDebug(debug) {}

  Impl(const PostgreSQLConnectionOptions& theConnectionOptions)
      : itsDebug(theConnectionOptions.debug), itsConnectionOptions(theConnectionOptions)
  {
    open(itsConnectionOptions);
  }

  bool open(const PostgreSQLConnectionOptions& theConnectionOptions)
  {
    itsConnectionOptions = theConnectionOptions;
    return reopen();
  }

  bool isTransaction() const { return !!itsTransaction; }

  void startTransaction() { itsTransaction = std::make_shared<pqxx::work>(*itsConnection); }
  void endTransaction() { itsTransaction.reset(); }
  void commitTransaction() { itsTransaction->commit(); }
  void exec(const std::string& sql) { itsTransaction->exec(sql); }

  void cancel()
  {
    if (itsConnection)
      itsConnection->cancel_query();
  }

  void close() const { itsConnection.reset(); }
  bool isConnected() const { return itsConnection->is_open(); }
  bool collateSupported() const { return itsCollate; }
  const std::map<unsigned int, std::string>& dataTypes() const { return itsDataTypes; }

  bool isDebug() const { return itsDebug; }
  void setDebug(bool debug) { itsDebug = debug; }

  void check_connection() const
  {
    if (!itsConnection || !isConnected())
      reopen();
  }

  void setClientEncoding(const std::string& theEncoding)
  {
    try
    {
      if (itsConnection)
        itsConnection->set_client_encoding(theEncoding);

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
      if (itsConnection)
        return itsConnection->quote(theString);
      throw Fmi::Exception(BCP, "Locus: Attempting to quote string without database connection");
    }
    catch (...)
    {
      throw Fmi::Exception::Trace(BCP, "Operation failed!");
    }
  }

  bool reopen() const
  {
    try
    {
      close();

      const std::string conn_str = itsConnectionOptions;

      std::string error_message;
      // Retriy connections automatically. Especially useful after boots if the database is in the
      // same server.
      for (auto retries = 10; retries > 0; --retries)
      {
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
          if (retries > 0)
          {
            std::string msg = e.what();
            boost::algorithm::replace_all(msg, "\n", " ");
            std::cerr << fmt::format("Warning: {} retries left. PG message: {}\n", retries, msg);
            std::this_thread::sleep_for(std::chrono::seconds(10));
          }
          else
            error_message = e.what();
        }
        catch (const std::exception& e)
        {
          error_message = e.what();
          break;
        }

        if (!error_message.empty())
          throw Fmi::Exception(BCP,
                               "Failed to connect to " + itsConnectionOptions.username + "@" +
                                   itsConnectionOptions.database + ":" +
                                   std::to_string(itsConnectionOptions.port) + " : " +
                                   error_message);

        // Store info of data types
        pqxx::result result_set = executeNonTransaction("select typname,oid from pg_type");
        for (auto row : result_set)
        {
          auto datatype = row[0].as<std::string>();
          unsigned int oid = row[1].as<unsigned int>();
          itsDataTypes.insert(std::make_pair(oid, datatype));
        }

        return itsConnection->is_open();
      }

      return false;
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

      check_connection();

      try
      {
        pqxx::nontransaction nitsTransaction(*itsConnection);
        return nitsTransaction.exec(theSQLStatement);
      }
      catch (const std::exception& e)
      {
        // Try reopening the connection only once not to flood the network
        if (itsConnection->is_open() || !reopen())
          throw Fmi::Exception(BCP,
                               std::string("Execution of SQL statement failed: ").append(e.what()));

        try
        {
          pqxx::nontransaction nitsTransaction(*itsConnection);
          return nitsTransaction.exec(theSQLStatement);
        }
        catch (const std::exception& e)
        {
          throw Fmi::Exception(BCP,
                               std::string("Execution of SQL statement failed: ").append(e.what()));
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
        check_connection();
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
    return impl->open(theConnectionOptions);
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
    return impl->reopen();
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
