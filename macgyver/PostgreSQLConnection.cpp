#include "PostgreSQLConnection.h"
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
  open(itsConnectionOptions);
}

bool PostgreSQLConnection::open(const PostgreSQLConnectionOptions& theConnectionOptions)
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
    throw std::runtime_error("Failed to connect to " + itsConnectionOptions.username + "@" +
                             itsConnectionOptions.database + ":" +
                             std::to_string(itsConnectionOptions.port) + " : " + e.what());
  }

  return itsConnection->is_open();
}

bool PostgreSQLConnection::reopen()
{
  close();
  return open(itsConnectionOptions);
}

void PostgreSQLConnection::close()
{
  if (!itsConnection) return;
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
		throw std::runtime_error(string("Failed to close connection to PostgreSQL: ") + e.what());
	  }
#endif
}

std::string PostgreSQLConnection::quote(const std::string& theString) const
{
  if (itsConnection) return itsConnection->quote(theString);
  throw std::runtime_error("Locus: Attempting to quote string without database connection");
}

pqxx::result PostgreSQLConnection::executeNonTransaction(const std::string& theSQLStatement) const
{
  if (itsDebug) std::cout << "SQL: " << theSQLStatement << std::endl;

  try
  {
    pqxx::nontransaction nitsTransaction(*itsConnection);
    return nitsTransaction.exec(theSQLStatement);
  }
  catch (const std::exception& e)
  {
    throw std::runtime_error(std::string("Execution of SQL statement failed: ").append(e.what()));
  }
}

void PostgreSQLConnection::startTransaction()
{
  itsTransaction = boost::make_shared<pqxx::work>(*itsConnection);
}

pqxx::result PostgreSQLConnection::executeTransaction(const std::string& theSQLStatement) const
{
  if (itsDebug) std::cout << "SQL: " << theSQLStatement << std::endl;

  try
  {
    return itsTransaction->exec(theSQLStatement);
  }
  catch (const std::exception& e)
  {
    throw std::runtime_error(
        std::string("Execution of SQL statement (transaction mode) failed: ").append(e.what()));
  }
}

void PostgreSQLConnection::commitTransaction()
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
    throw std::runtime_error(std::string("Commiting transaction failed: ").append(e.what()));
  }
}

void PostgreSQLConnection::setClientEncoding(const std::string& theEncoding) const
{
  try
  {
    itsConnection->set_client_encoding(theEncoding);
  }
  catch (const std::exception& e)
  {
    throw std::runtime_error(std::string("set_client_encoding failed: ").append(e.what()));
  }
}

}  // namespace Database
}  // namespace Fmi

// ======================================================================
