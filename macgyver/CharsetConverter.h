#pragma once

#include <memory>
#include <string>
#include <boost/noncopyable.hpp>

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

}
