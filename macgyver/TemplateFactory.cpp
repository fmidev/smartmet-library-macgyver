// ======================================================================

#include "TemplateFactory.h"
#include <boost/filesystem/operations.hpp>
#include <boost/make_shared.hpp>

namespace Fmi
{
// CT++ may not be thread safe - but using a thread specific
// storage for cached copies makes using it thread safe

struct TemplateInfo
{
  std::time_t modtime;
  SharedFormatter formatter;

  TemplateInfo() : modtime(0), formatter() {}
};

using TemplateMap = std::map<boost::filesystem::path, TemplateInfo>;
thread_local TemplateMap itsTemplates{};

// ----------------------------------------------------------------------
/*!
 * \brief Get a thread specific template formatter for given template
 */
// ----------------------------------------------------------------------

SharedFormatter TemplateFactory::get(const boost::filesystem::path& theFilename) const
{
  if (theFilename.empty()) throw std::runtime_error("TemplateFactory: Cannot use empty templates");

  const auto& tinfo = itsTemplates.find(theFilename);

  const std::time_t modtime = boost::filesystem::last_write_time(theFilename);

  // Use cached template if it is up to date
  if (tinfo != itsTemplates.end())
    if (tinfo->second.modtime == modtime) return tinfo->second.formatter;

  // Initialize a new formatter

  TemplateInfo newinfo;
  newinfo.modtime = modtime;
  newinfo.formatter = boost::make_shared<Fmi::TemplateFormatter>();
  newinfo.formatter->load_template(theFilename.c_str());

  // Cache the new formatter
  itsTemplates.insert(std::make_pair(theFilename, newinfo));

  return newinfo.formatter;
}

}  // namespace Fmi
