/*
 * TemplateFormatter.cpp
 *
 *  Created on: Oct 4, 2012
 *      Author: pavenis
 */

#include "TemplateFormatter.h"
#include <ctpp2/CTPP2Logger.hpp>
#include <ctpp2/CTPP2OutputCollector.hpp>
#include <ctpp2/CTPP2VM.hpp>
#include <ctpp2/CTPP2VMFileLoader.hpp>
#include <ctpp2/CTPP2VMSTDLib.hpp>
#include <cstdarg>
#include <cstdio>
#include <ostream>
#include <string>

Fmi::TemplateFormatter::~TemplateFormatter() {}

class Fmi::TemplateFormatter::OutputCollector : public CTPP::OutputCollector
{
  std::ostream& ost;

 public:
  OutputCollector(std::ostream& the_ost) : ost(the_ost) {}
  virtual ~OutputCollector() throw();

  virtual INT_32 Collect(const void* vData, const UINT_32 iDataLength)
  {
    ost.write(reinterpret_cast<const char*>(vData), iDataLength);
    return 0;
  }
};

Fmi::TemplateFormatter::OutputCollector::~OutputCollector() throw() {}
/**
 *  @brief CTPP logger class for writing log messages to provided std::ostream
 */
class Fmi::TemplateFormatter::Logger : public CTPP::Logger
{
  std::ostream& ost;

 public:
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

  // iBasePriority is used but clang does not see it
  Logger(std::ostream& the_ost, const UINT_32 iIBasePriority)
      : CTPP::Logger(iBasePriority), ost(the_ost)
  {
  }

#pragma clang diagnostic pop

  virtual ~Logger() throw();

  virtual INT_32 WriteLog(const UINT_32 iPriority, CCHAR_P szString, const UINT_32 iStringLen)
  {
    if (iPriority >= iBasePriority)
    {
      // FIXME: shouldn't reject for too long messages
      ost.write(szString, iStringLen);
    }

    return 0;
  }
};

Fmi::TemplateFormatter::Logger::~Logger() throw() {}
Fmi::TemplateFormatter::TemplateFormatter(UINT_32 max_handlers) : syscall_factory(max_handlers)
{
  CTPP::STDLibInitializer::InitLibrary(syscall_factory);
}

int Fmi::TemplateFormatter::process(CTPP::CDT& hash,
                                    std::ostream& output_stream,
                                    std::ostream& log_stream)
{
  return process(hash, output_stream, log_stream, CTPP2_LOG_WARNING);
}

/*
 *
 * CTPP2_LOG_EMERG 0    system is unusable
 * CTPP2_LOG_ALERT 1    action must be taken immediately
 * CTPP2_LOG_CRIT 2     critical conditions
 * CTPP2_LOG_ERR 3      error conditions
 * CTPP2_LOG_WARNING 4  warning conditions
 * CTPP2_LOG_NOTICE 5   normal but significant condition
 * CTPP2_LOG_INFO 6     informational
 * CTPP2_LOG_DEBUG 7    debug-level messages
 */

int Fmi::TemplateFormatter::process(CTPP::CDT& hash,
                                    std::ostream& output_stream,
                                    std::ostream& log_stream,
                                    int log_level)
{
  // Create output collector for provided output stream
  TemplateFormatter::OutputCollector output_collector(output_stream);

  // Create virtual machine for template processing
  CTPP::VM tmp_vm(&syscall_factory, 4096, 4096, 0x40000000, log_level);

  // Create logger for writing log messages to the output stream
  TemplateFormatter::Logger logger(log_stream, log_level);

  // Get data to run on CTPP2 virtual machine
  const CTPP::VMMemoryCore* p_memory_core = loader->GetCore();

  // Initialize CTPP2 virtual machine for output formatting.
  tmp_vm.Init(p_memory_core, &output_collector, &logger);

  // Finally process the template and generate output
  UINT_32 iIP = 0;
  return tmp_vm.Run(p_memory_core, &output_collector, iIP, hash, &logger);
}

void Fmi::TemplateFormatter::load_template(const std::string& file_name)
{
  loader.reset(new CTPP::VMFileLoader(file_name.c_str()));
}
