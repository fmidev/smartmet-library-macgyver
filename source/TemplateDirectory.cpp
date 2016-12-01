#include "TemplateDirectory.h"
#include "TypeName.h"

#include <sstream>
#include <stdexcept>

namespace fs = boost::filesystem;

Fmi::TemplateDirectory::TemplateDirectory(const fs::path& the_template_dir)
    : template_dir(the_template_dir)
{
  if (not fs::exists(template_dir))
  {
    std::ostringstream msg;
    msg << METHOD_NAME << ": directory '" << template_dir << "' is not found";
    throw std::runtime_error(msg.str());
  }

  if (not fs::is_directory(template_dir))
  {
    std::ostringstream msg;
    msg << METHOD_NAME << ": '" << template_dir << "' is not a directory";
    throw std::runtime_error(msg.str());
  }
}

Fmi::TemplateDirectory::~TemplateDirectory() {}
fs::path Fmi::TemplateDirectory::find_template(const std::string& name) const
{
  fs::path p;
  fs::path p0(name);
  if (p.is_absolute())
  {
    p = p0;
  }
  else
  {
    p = template_dir;
    p /= p0;
  }

  if (not fs::exists(p))
    throw std::runtime_error(std::string("Stored query template"
                                         " file '") +
                             p.string() + "' is not found");

  return p;
}

boost::shared_ptr<Fmi::TemplateFormatterMT> Fmi::TemplateDirectory::create_template_formatter(
    const std::string& name) const
{
  const fs::path fn = find_template(name);
  boost::shared_ptr<Fmi::TemplateFormatterMT> formatter(new Fmi::TemplateFormatterMT(fn.string()));
  return formatter;
}
