#include "PostgreSQLConnection.h"
#include "Exception.h"
#include "TypeName.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/variant.hpp>
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

PostgreSQLConnection::PostgreSQLConnection(const PostgreSQLConnectionOptions& theConnectionOptions)
    : itsDebug(theConnectionOptions.debug),
      itsCollate(false),
      itsConnectionOptions(theConnectionOptions)
{
  try
  {
    open(itsConnectionOptions);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

bool PostgreSQLConnection::open(const PostgreSQLConnectionOptions& theConnectionOptions)
{
  try
  {
    itsConnectionOptions = theConnectionOptions;

    close();

    const std::string conn_str = itsConnectionOptions;

    try
    {
      itsConnection = boost::make_shared<pqxx::connection>(conn_str);
      /*
        if(PostgreSQL > 9.1)
        itsCollate = true;
        pqxx::result res = executeNonTransaction("SELECT version()");
      */
    }
    catch (const std::exception& e)
    {
      throw Fmi::Exception(BCP,
                           "Failed to connect to " + itsConnectionOptions.username + "@" +
                               itsConnectionOptions.database + ":" +
                               std::to_string(itsConnectionOptions.port) + " : " + e.what());
    }

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
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

bool PostgreSQLConnection::reopen()
{
  try
  {
    close();
    return open(itsConnectionOptions);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

void PostgreSQLConnection::close()
{
  try
  {
    if (!itsConnection)
      return;

#if PQXX_VERSION_MAJOR < 7
    itsConnection->disconnect();
#else
    // TODO:checked whether this works with libpqxx7
    try
    {
      if (itsConnection->is_open())
        itsConnection->close();
      itsConnection.reset();
    }
    catch (const std::exception& e)
    {
      throw Fmi::Exception(BCP,
                           std::string("Failed to close connection to PostgreSQL: ") + e.what());
    }
#endif
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
    if (itsConnection)
      return itsConnection->quote(theString);

    throw Fmi::Exception(BCP, "Locus: Attempting to quote string without database connection");
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

pqxx::result PostgreSQLConnection::executeNonTransaction(const std::string& theSQLStatement) const
{
  // FIXME: should we fail if transaction is active?
  try
  {
    if (itsDebug)
      std::cout << "SQL: " << theSQLStatement << std::endl;

    try
    {
      pqxx::nontransaction nitsTransaction(*itsConnection);
      return nitsTransaction.exec(theSQLStatement);
    }
    catch (const std::exception& e)
    {
      // Try reopening the connection only once not to flood the network
      if (itsConnection->is_open() || !const_cast<PostgreSQLConnection*>(this)->reopen())
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

pqxx::result PostgreSQLConnection::execute(const std::string& theSQLStatement) const
{
  try
  {
    if (itsTransaction)
      return itsTransaction->exec(theSQLStatement);

    return executeNonTransaction(theSQLStatement);
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
    if (itsConnection)
    {
      itsConnection->cancel_query();
    }
  }
  catch (const std::exception& e)
  {
    throw std::runtime_error(std::string("Canceling transaction failed: ").append(e.what()));
  }
}

void PostgreSQLConnection::setClientEncoding(const std::string& theEncoding)
{
  try
  {
    try
    {
      if (itsConnection)
      {
        itsConnection->set_client_encoding(theEncoding);
      }
      itsConnectionOptions.encoding = theEncoding;
    }
    catch (const std::exception& e)
    {
      throw Fmi::Exception(BCP, std::string("set_client_encoding failed: ").append(e.what()));
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

PostgreSQLConnection::Transaction::Transaction(PostgreSQLConnection& conn) : conn(conn)
{
  try
  {
    if (conn.itsTransaction)
      throw Fmi::Exception(BCP, "Recursive transactions are not supported");

    conn.itsTransaction = boost::make_shared<pqxx::work>(*conn.itsConnection);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

PostgreSQLConnection::Transaction::~Transaction()
{
  conn.itsTransaction.reset();
}

pqxx::result PostgreSQLConnection::Transaction::execute(const std::string& theSQLStatement) const
{
  try
  {
    if (conn.itsTransaction)
    {
      if (conn.itsDebug)
      {
        std::cout << "SQL: " << theSQLStatement << std::endl;
      }
      return conn.itsTransaction->exec(theSQLStatement);
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
    if (conn.itsTransaction)
    {
      try
      {
        conn.itsTransaction->commit();
        conn.itsTransaction.reset();
      }
      catch (const std::exception& e)
      {
        // If we get here, Xaction has been rolled back
        conn.itsTransaction.reset();
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
    if (conn.itsTransaction)
    {
      conn.itsTransaction.reset();
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
