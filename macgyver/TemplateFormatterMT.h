#pragma once

#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include "TemplateFormatter.h"

namespace Fmi
{
/**
 *   @brief Wrapper for ensuring separate per thread instance of TemplateFormatter
 */
class TemplateFormatterMT : public boost::noncopyable
{
 public:
  TemplateFormatterMT(const std::string& file_name);

  virtual ~TemplateFormatterMT();

  TemplateFormatter* get();

 private:
  const std::string file_name;
  boost::thread_specific_ptr<TemplateFormatter> tf_mt;
};
}
