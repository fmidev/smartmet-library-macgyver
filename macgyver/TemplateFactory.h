// ======================================================================
/*!
 * \brief A factory for thread safe template formatting
 */
// ======================================================================

#pragma once

#include "TemplateFormatter.h"
#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

namespace Fmi
{
using SharedFormatter = boost::shared_ptr<Fmi::TemplateFormatter>;

class TemplateFactory : public boost::noncopyable
{
 public:
  SharedFormatter get(const boost::filesystem::path& theFilename) const;

};  // class TemplateFactory

}  // namespace Fmi
