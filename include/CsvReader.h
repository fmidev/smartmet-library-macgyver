// ======================================================================
/*!
 * \brief CSV file reader
 */
// ======================================================================

#pragma once

#include <boost/function.hpp>
#include <string>
#include <vector>

namespace Fmi
{
namespace CsvReader
{
typedef std::vector<std::string> row_type;
typedef boost::function<void(const row_type& row)> Callback;

void read(const std::string& filename, Callback callback, char delimiter = ',');
}
}

// ======================================================================
