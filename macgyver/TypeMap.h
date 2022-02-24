#pragma once

#include <map>
#include <typeinfo>
#include <typeindex>
#include <boost/any.hpp>
#include <boost/variant.hpp>
#include "Exception.h"
#include "TypeName.h"

namespace Fmi
{

/**
 *   @brief A template class to associate object with specified types
 *
 *   Intended mostly for use with boost::any and boost::variant which can
 *   contain data of different types to avoid long if then else chains
 *
 *   An example for use with std::function<std::string(const boost::any&) kanssa:
 *   @code
 *   TypeMap<std::function<std::string(const boost::any&)> > W;
 *   W.add<int>(
 *        [](const boost::any& x) -> std::string
 *        {
 *            return "INT: " + std::to_string(boost::any_cast<int>(x));
 *        });
 *    W.add<std::string>(
 *        [](const boost::any& x) -> std::string
 *        {
 *            return "STRING: '" + boost::any_cast<std::string>(x) + "'";
 *        });
 *    boost::any foo = int(1), bar(std::string("bar"));
 *    std::cout << W[foo](foo) << std::endl;
 *    std::cout << W[bar](bar) << std::endl;
 *
 *   @endcode
 *
 *   Similar example for boost::variant<>:
 *   @code
 *    TypeMap<std::function<std::string(const boost::variant<int, std::string>&)> > W;
 *    W.add<int>([](
 *            const boost::variant<int, std::string> x) -> std::string
 *            {
 *                return "INT: " + std::to_string(boost::get<int>(x));
 *            });
 *    W.add<std::string>([](
 *            const boost::variant<int, std::string> x) -> std::string
 *            {
 *                return "STRING: '" + boost::get<std::string>(x) + "'";
 *            });
 *    boost::variant<int, std::string> foo = int(42), bar = "24";
 *    std::cout << W[foo](foo) << std::endl;
 *    std::cout << W[bar](bar) << std::endl;
 *   @edcode
 */
template <typename ValueType>
class TypeMap
{
public:
    TypeMap() = default;
    virtual ~TypeMap() = default;

    template <typename Type>
    TypeMap& add(const ValueType& value)
    {
        content[std::type_index(typeid(Type))] = value;
        return *this;
    }

    const ValueType& operator [] (const std::type_info& ti) const
    {
        auto it = content.find(std::type_index(ti));
        if (it != content.end()) {
            return it->second;
        } else {
            throw Exception(BCP, "Fmi::TypeMap: No value provided for type "
                + demangle_cpp_type_name(ti.name()));
        }
    };

    const ValueType& operator [] (const boost::any& x) const
    {
        return operator [] (x.type());
    };

    template <class... VariantTypes>
    const ValueType& operator [] (const boost::variant<VariantTypes...>& x) const
    {
        return operator [] (x.type());
    };

private:
    std::map<std::type_index, ValueType> content;
};

} // namespace Fmi
