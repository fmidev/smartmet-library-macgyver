#include "TemplateFormatterMT.h"

Fmi::TemplateFormatterMT::TemplateFormatterMT(const std::string& the_file_name)
    : file_name(the_file_name), tf_mt()
{
}

Fmi::TemplateFormatterMT::~TemplateFormatterMT() {}
Fmi::TemplateFormatter* Fmi::TemplateFormatterMT::get()
{
  Fmi::TemplateFormatter* result = tf_mt.get();
  if (result == NULL)
  {
    tf_mt.reset(new Fmi::TemplateFormatter);
    result = tf_mt.get();
    result->load_template(file_name);
  }
  return result;
}
