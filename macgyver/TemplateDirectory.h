#pragma once

#include <string>
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include "TemplateFormatterMT.h"

namespace Fmi
{
/**
 *  @brief Provides template lookup from specified directory and
 *         creating BarinStorm::TemplateFormatterMT objects
 */
class TemplateDirectory
{
 public:
  TemplateDirectory(const boost::filesystem::path& template_dir);

  virtual ~TemplateDirectory();

  boost::filesystem::path find_template(const std::string& name) const;

  boost::shared_ptr<TemplateFormatterMT> create_template_formatter(const std::string& name) const;

 private:
  const boost::filesystem::path template_dir;
};
}
