#include "Exception.h"
#include "AnsiEscapeCodes.h"
#include "StringConversion.h"
#include "TypeName.h"
#include <boost/thread.hpp>
#include <fmt/format.h>
#include <iostream>

namespace Fmi
{
namespace
{
static const std::map<std::string, std::string> exception_name_map = {
    {"std::out_of_range", "Out of range"},
    {"std::invalid_argument", "Invalid argument"},
    {"std::length_error", "Length error"},
    {"std::domain_error", "Domain error"},
    {"std::range_error", "Range error"},
    {"std::overflow_error", "Overflow error"},
    {"std::underflow_error", "Underflow error"},
    {"std::runtime_error", "Runtime error"},
    {"std::logic_error", "Logic error"}};
}

std::atomic<bool> Exception::force_stack_trace(false);

Exception::Exception()
{
  timestamp = boost::posix_time::second_clock::local_time();
  line = 0;
}

Exception::Exception(const Exception& other)
{
  timestamp = other.timestamp;
  filename = other.filename;
  line = other.line;
  function = other.function;
  message = other.message;
  detailVector = other.detailVector;
  parameterVector = other.parameterVector;
  prevException = nullptr;
  mStackTraceDisabled = other.mStackTraceDisabled;
  mLoggingDisabled = other.mLoggingDisabled;
  prevException = other.prevException;
}

Exception Exception::Trace(const char* _filename,
                           int _line,
                           const char* _function,
                           std::string _message)
{
  return Exception(_filename, _line, _function, std::move(_message), nullptr);
}

Exception::Exception(const char* _filename,
                     int _line,
                     const char* _function,
                     std::string _message,
                     Exception* _prevException)
{
  timestamp = boost::posix_time::second_clock::local_time();
  filename = _filename;
  line = _line;
  function = _function;
  message = std::move(_message);
  prevException = nullptr;
  if (_prevException != nullptr)
  {
    prevException.reset(new Exception(*_prevException));
  }
  else if (std::current_exception())
  {
    try
    {
      throw;
    }
    catch (Fmi::Exception& e)
    {
      prevException.reset(new Exception(e));
      // Propagate the flags to the top
      mStackTraceDisabled = e.mStackTraceDisabled;
      mLoggingDisabled = e.mLoggingDisabled;
    }
    catch (const boost::thread_interrupted&)
    {
      throw;
    }
    catch (std::exception& e)
    {
      const std::string cxx_name = Fmi::get_type_name(&e);
      auto it = exception_name_map.find(cxx_name);
      const std::string print_name = it == exception_name_map.end() ? cxx_name : it->second;
      prevException.reset(new Exception(
          _filename, _line, _function, std::string("[") + print_name + "] " + e.what()));
    }
    catch (...)
    {
      prevException.reset(new Exception(
          _filename, _line, _function, std::string("[") + Fmi::current_exception_type() + "]"));
    }
  }
}

Exception::Exception(const char* _filename, int _line, const char* _function, std::string _message)
{
  timestamp = boost::posix_time::second_clock::local_time();
  filename = _filename;
  line = _line;
  function = _function;
  message = std::move(_message);
}

Exception::~Exception() {}

std::string Exception::getFilename() const
{
  return filename;
}

int Exception::getLine() const
{
  return line;
}

std::string Exception::getFunction() const
{
  return function;
}

const char* Exception::getWhat() const noexcept(true)
{
  return message.c_str();
}

const char* Exception::what() const noexcept(true)
{
  return getFirstException()->getWhat();
}

const Exception* Exception::getPrevException() const
{
  if (prevException)
  {
    return prevException.get();
  }
  else
  {
    return nullptr;
  }
}

const Exception* Exception::getFirstException() const
{
  if (prevException)
  {
    return prevException->getFirstException();
  }
  else
  {
    return this;
  }
}

ExceptionTimeStamp Exception::getTimeStamp() const
{
  return timestamp;
}

unsigned int Exception::getExceptionCount() const
{
  unsigned int count = 0;
  const Exception* e = this;
  while (e != nullptr)
  {
    count++;
    e = e->getPrevException();
  }
  return count;
}

const Exception* Exception::getExceptionByIndex(unsigned int _index) const
{
  unsigned int count = 0;
  const Exception* e = this;
  while (e != nullptr)
  {
    if (count == _index)
      return e;

    count++;
    e = e->getPrevException();
  }
  return nullptr;
}

void Exception::setTimeStamp(ExceptionTimeStamp _timestamp)
{
  timestamp = _timestamp;
}

Exception& Exception::addDetail(std::string _detailStr)
{
  detailVector.emplace_back(_detailStr);
  return *this;
}

Exception& Exception::addDetails(const DetailList& _detailList)
{
  auto begin = _detailList.begin();
  auto end = _detailList.end();

  while (begin != end)
    addDetail(*begin++);

  return *this;
}

Exception& Exception::addParameter(const char* _name, std::string _value)
{
  parameterVector.push_back(std::make_pair(std::string(_name), _value));
  return *this;
}

const char* Exception::getDetailByIndex(unsigned int _index) const
{
  if (_index < getDetailCount())
    return detailVector.at(_index).c_str();

  return nullptr;
}

const char* Exception::getParameterNameByIndex(unsigned int _index) const
{
  if (_index < getParameterCount())
  {
    const auto& p = parameterVector.at(_index);
    return p.first.c_str();
  }

  return nullptr;
}

const char* Exception::getParameterValue(const char* _name) const
{
  size_t size = parameterVector.size();
  if (size > 0)
  {
    for (size_t t = 0; t < size; t++)
    {
      const auto& p = parameterVector.at(t);
      if (p.first == _name)
        return p.second.c_str();
    }
  }
  return nullptr;
}

const char* Exception::getParameterValueByIndex(unsigned int _index) const
{
  if (_index < getParameterCount())
  {
    const auto& p = parameterVector.at(_index);
    return p.second.c_str();
  }

  return nullptr;
}

const Exception* Exception::getExceptionByParameterName(const char* _paramName) const
{
  if (getParameterValue(_paramName) != nullptr)
    return this;

  if (prevException == nullptr)
    return nullptr;

  return prevException->getExceptionByParameterName(_paramName);
}

unsigned int Exception::getDetailCount() const
{
  return static_cast<unsigned int>(detailVector.size());
}

unsigned int Exception::getParameterCount() const
{
  return static_cast<unsigned int>(parameterVector.size());
}

bool Exception::loggingDisabled() const
{
  return mLoggingDisabled;
}

bool Exception::stackTraceDisabled() const
{
  return mStackTraceDisabled;
}

Exception& Exception::disableLogging()
{
  mLoggingDisabled = true;
  return *this;
}

Exception& Exception::disableStackTrace()
{
  mStackTraceDisabled = true;
  return *this;
}

std::string Exception::getStackTrace() const
{
  if (!force_stack_trace && (mLoggingDisabled || mStackTraceDisabled))
    return "";

  const Exception* e = this;

  std::string out = fmt::format("\n{}{}{} #### {} #### {}{}{}\n\n",
                                ANSI_BG_RED,
                                ANSI_FG_WHITE,
                                ANSI_BOLD_ON,
                                Fmi::to_iso_string(e->timestamp),
                                ANSI_BOLD_OFF,
                                ANSI_FG_DEFAULT,
                                ANSI_BG_DEFAULT);

  while (e != nullptr)
  {
    out += fmt::format("{}{}EXCEPTION {}{}{}\n{} * Function   : {}{}\n{} * Location   : {}{}:{}\n",
                       ANSI_FG_RED,
                       ANSI_BOLD_ON,
                       ANSI_BOLD_OFF,
                       e->message,
                       ANSI_FG_DEFAULT,
                       ANSI_BOLD_ON,
                       ANSI_BOLD_OFF,
                       e->function,
                       ANSI_BOLD_ON,
                       ANSI_BOLD_OFF,
                       e->filename,
                       e->line);

    size_t size = e->detailVector.size();
    if (size > 0)
    {
      out += ANSI_BOLD_ON;
      out += " * Details    :\n";
      out += ANSI_BOLD_OFF;
      for (size_t t = 0; t < size; t++)
      {
        out += "   - ";
        out += e->detailVector.at(t);
        out += "\n";
      }
    }

    size = e->parameterVector.size();
    if (size > 0)
    {
      out += ANSI_BOLD_ON;
      out += " * Parameters :\n";
      out += ANSI_BOLD_OFF;
      for (size_t t = 0; t < size; t++)
      {
        auto p = e->parameterVector.at(t);
        out += "   - ";
        out += p.first;
        out += " = ";
        out += p.second;
        out += "\n";
      }
    }
    out += "\n";

    e = e->prevException.get();
  }

  return out;
}

std::string Exception::getHtmlStackTrace() const
{
  // This is used only when debugging, hence mStackTraceDisabled is ignored

  std::string out = "<html><body><h2>" + Fmi::to_iso_string(timestamp) + "</h2>";

  const Exception* e = this;
  while (e != nullptr)
  {
    out += fmt::format(
        "<h2>{}</h2><ul><li><it>Function :</it>{}</li><li><it>Location :</it>{}:{}</li>",
        e->message,
        e->function,
        e->filename,
        e->line);

    size_t size = e->detailVector.size();
    if (size > 0)
    {
      out += "<li><it>Details :</it></li>";
      out += "<ol>";
      for (size_t t = 0; t < size; t++)
      {
        out += "<li>";
        out += e->detailVector.at(t);
        out += "</li>";
      }
      out += "</ol>";
    }

    size = e->parameterVector.size();
    if (size > 0)
    {
      out += "<li><it>Parameters :</it>";
      out += "<ol>";
      for (size_t t = 0; t < size; t++)
      {
        auto p = e->parameterVector.at(t);
        out += "<li>";
        out += p.first;
        out += " = ";
        out += p.second;
        out += "</li>";
      }
      out += "</ol></li>";
    }
    out += "</ul>";

    e = e->prevException.get();
  }

  out += "</body></html>";

  return out;
}

// ----------------------------------------------------------------------
/*!
 * \brief Print the exception error report while obeying the user settings
 */
// ----------------------------------------------------------------------

void Exception::printError() const
{
  if (!stackTraceDisabled())
    std::cerr << getStackTrace() << std::flush;
  else
  {
#if 0
    // Disabled for now due to too many trivial client errors. We need better
    // control of error logging and preferably on-the-fly modifications to it.
    
    std::cerr << Spine::log_time_str() << " Error: " << what() << std::endl;

    // Print parameters for top level exception, if there are any. Usually
    // plugins set the URI to the top level exception.

    if (!parameterVector.empty())
    {
      for (const auto& param_value : parameterVector)
        std::cerr << "   - " << param_value.first << " = " << param_value.second << std::endl;
    }
#endif
  }
}

}  // namespace Fmi
