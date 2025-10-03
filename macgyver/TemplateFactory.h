// ======================================================================
/*!
 * \brief A factory for thread safe template formatting
 */
// ======================================================================

#pragma once

#include "TemplateFormatter.h"
#include <boost/shared_ptr.hpp>
#include <filesystem>

namespace Fmi
{
using SharedFormatter = std::shared_ptr<Fmi::TemplateFormatter>;

namespace TemplateFactory
{

SharedFormatter get(const std::filesystem::path& theFilename);

}  // namespace TemplateFactory

}  // namespace Fmi
