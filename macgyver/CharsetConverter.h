#pragma once

#include <boost/noncopyable.hpp>
#include <memory>
#include <string>

namespace Fmi
{
class CharsetConverter : public boost::noncopyable
{
 public:
  CharsetConverter(const std::string& from, const std::string& to, std::size_t max_len = 0);
  virtual ~CharsetConverter();

  std::string convert(const std::string& src) const;

 private:
  struct Impl;

  const std::string from;
  const std::string to;
  const std::size_t max_len;
  std::unique_ptr<Impl> impl;
};

}  // namespace Fmi
