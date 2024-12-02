#pragma once

#include <functional>
#include <map>
#include <initializer_list>
#include "Exception.h"

namespace Fmi
{
    /**
     *  @brief A template class to associate functions with specified names
     *
     *  Intended mostly for use with std::function which can contain functions
     */
    template <typename ValueType, class... ArgumentTypes>
    class FunctionMap
    {
        std::map<std::string, std::function<ValueType(ArgumentTypes...)>> m_map;

    public:
        FunctionMap() = default;

        FunctionMap(const FunctionMap&) = default;

        FunctionMap(
            const std::initializer_list<
                std::pair<
                    std::string,
                    std::function<ValueType(ArgumentTypes...)>
                >
            > functions)
        {
            for (const auto& [name, function] : functions)
            {
                if (!function)
                    report_empty_function( { name } );
                m_map[name] = function;
            }
        }

        FunctionMap(
            const std::initializer_list<
                std::pair<
                    const std::initializer_list<std::string>,
                    std::function<ValueType(ArgumentTypes...)>
                >
            > functions)
        {
            for (const auto& [names, function] : functions)
            {
                if (!function)
                    report_empty_function( names );
                for (const auto& name : names)
                {
                    m_map[name] = function;
                }
            }
        }

        FunctionMap& operator=(const FunctionMap&) = default;

        FunctionMap& add(
            const std::string& name,
            std::function<ValueType(ArgumentTypes...)> function)
        {
            if (!function)
            {
                report_empty_function( { name } );
            }
            m_map[name] = function;
            return *this;
        }

        FunctionMap& add(
            const std::vector<std::string>& names,
            std::function<ValueType(ArgumentTypes...)> function)
        {
            if (!function)
            {
                report_empty_function(names);
            }

            for (const auto& name : names)
            {
                m_map[name] = function;
            }
            return *this;
        }

        bool contains(const std::string& name) const
        {
            return m_map.find(name) != m_map.end();
        }

        ValueType operator()(const std::string& name, ArgumentTypes... args) const
        {
            auto it = m_map.find(name);
            if (it != m_map.end())
            {
                return it->second(args...);
            }
            else
            {
                throw std::runtime_error("FunctionMap: No function named '" + name + "'");
            }
        }

        virtual ~FunctionMap() = default;

    private:
        void report_empty_function(const std::vector<std::string>& names) const
        {
            Fmi::Exception error(BCP, "FunctionMap: Function is null");
            std::string names_str;
            for (const auto& name : names)
            {
                names_str += (names_str == "" ? "" : ", ") + name;
            }
            error.addParameter("names", names_str);
            throw error;
        }
    };

}  // namespace Fmi
