// ======================================================================
/*!
 * \brief A factory for thread safe template formatting
 */
// ======================================================================

#pragma once

#include "TemplateFormatter.h"
#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>

namespace Fmi
{
using SharedFormatter = boost::shared_ptr<Fmi::TemplateFormatter>;

class TemplateFactory
{
 public:
  SharedFormatter get(const boost::filesystem::path& theFilename) const;

  TemplateFactory(const TemplateFactory& other) = delete;
  TemplateFactory(TemplateFactory&& other) = delete;
  TemplateFactory& operator=(const TemplateFactory& other) = delete;
  TemplateFactory& operator=(TemplateFactory&& other) = delete;

};  // class TemplateFactory

}  // namespace Fmi
