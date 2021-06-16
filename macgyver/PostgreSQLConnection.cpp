#include "PostgreSQLConnection.h"
#include "Exception.h"
#include <boost/make_shared.hpp>
#include <iostream>

namespace Fmi
{
namespace Database
{
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

    std::stringstream ss;

    // clang-format off
    ss << "host="      << itsConnectionOptions.host
       << " dbname="   << itsConnectionOptions.database
       << " port= "    << itsConnectionOptions.port
       << " user="     << itsConnectionOptions.username
       << " password=" << itsConnectionOptions.password
#if 0
       << " client_encoding=" << itsConnectionOptions.encoding
#endif
       ;
    // clang-format on

    if (itsConnectionOptions.connect_timeout > 0)
      ss << " connect_timeout=" << itsConnectionOptions.connect_timeout;

    try
    {
      itsConnection = boost::make_shared<pqxx::connection>(ss.str());
      /*
        if(PostgreSQL > 9.1)
        itsCollate = true;
        pqxx::result res = executeNonTransaction("SELECT version()");
      */
    }
    catch (const std::exception& e)
    {
      throw Fmi::Exception(BCP, "Failed to connect to " + itsConnectionOptions.username + "@" +
                               itsConnectionOptions.database + ":" +
                               std::to_string(itsConnectionOptions.port) + " : " + e.what());
    }

    // Store info of data types
    pqxx::result result_set = executeNonTransaction("select typname,oid from pg_type");
    for (auto row : result_set)
    {
      std::string datatype = row[0].as<std::string>();
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

    itsConnection->disconnect();

#if 0
    // disconnect does not throw according to documentation
    try
    {
      if(itsConnection->is_open())
        itsConnection->disconnect();
    }
    catch(const std::exception& e)
    {
      throw Fmi::Exception(BCP, string("Failed to close connection to PostgreSQL: ") + e.what());
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
      throw Fmi::Exception(BCP, std::string("Execution of SQL statement failed: ").append(e.what()));
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

void PostgreSQLConnection::startTransaction()
{
  try
  {
    itsTransaction = boost::make_shared<pqxx::work>(*itsConnection);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

pqxx::result PostgreSQLConnection::executeTransaction(const std::string& theSQLStatement) const
{
  try
  {
    if (itsDebug)
      std::cout << "SQL: " << theSQLStatement << std::endl;

    try
    {
      return itsTransaction->exec(theSQLStatement);
    }
    catch (const std::exception& e)
    {
      throw Fmi::Exception(BCP,
          std::string("Execution of SQL statement (transaction mode) failed: ").append(e.what()));
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

void PostgreSQLConnection::commitTransaction()
{
  try
  {
    try
    {
      itsTransaction->commit();
      itsTransaction.reset();
    }
    catch (const std::exception& e)
    {
      // If we get here, Xaction has been rolled back
      itsTransaction.reset();
      throw Fmi::Exception(BCP, std::string("Commiting transaction failed: ").append(e.what()));
    }
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
    itsConnection->cancel_query();
  }
  catch (const std::exception& e)
  {
    throw std::runtime_error(std::string("Canceling transaction failed: ").append(e.what()));
  }
}

void PostgreSQLConnection::setClientEncoding(const std::string& theEncoding) const
{
  try
  {
    try
    {
      itsConnection->set_client_encoding(theEncoding);
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

}  // namespace Database
}  // namespace Fmi

// ======================================================================
