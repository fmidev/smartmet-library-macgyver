#pragma once

#include <functional>
#include <map>
#include <regex>
#include <vector>
#include <initializer_list>
#include "Exception.h"

namespace Fmi
{
    /**
     *  @brief A template class to associate functions with specified names
     *
     *  It is recommended to use exact match when possible for better performance
     *  rather than regex matching.
     */
    template <typename ValueType, class... ArgumentTypes>
    class FunctionMap
    {
        struct Entry
        {
            std::function<ValueType(ArgumentTypes...)> function;
            std::string description;
            Entry() = default;
            Entry(
                std::function<ValueType(ArgumentTypes...)> function,
                const std::string& description)
                : function(function), description(description)
            {
            }
        };

        struct RegexEntry
        {
            const std::string name;
            std::regex regex;
            std::function<ValueType(const std::vector<std::string>&, ArgumentTypes...)> function;
            std::string description;
            RegexEntry() = default;
            RegexEntry(const std::string& name,
                       const std::regex& regex,
                       std::function<ValueType(std::vector<std::string>, ArgumentTypes...)> function,
                       const std::string& description)
                : name(name), regex(regex), function(function), description(description)
            {
            }
        };

        std::map<std::string, Entry> m_map;
        std::vector<RegexEntry> m_regex_entries;

    public:
        FunctionMap() = default;

        FunctionMap(const FunctionMap&) = default;

        FunctionMap& operator=(const FunctionMap&) = default;

        FunctionMap& add(
            const std::string& name,
            std::function<ValueType(ArgumentTypes...)> function,
            const std::string& description = "")
        {
            if (!function)
            {
                report_empty_function( { name } );
            }
            m_map.emplace(name, Entry(function, description));
            return *this;
        }

        FunctionMap& add(
            const std::vector<std::string>& names,
            std::function<ValueType(ArgumentTypes...)> function,
            const std::string& description = "")
        {
            if (!function)
            {
                report_empty_function(names);
            }

            for (const auto& name : names)
            {
                m_map.emplace(name, Entry(function, description));
            }
            return *this;
        }

        FunctionMap& add(
            const std::string& name,
            const std::regex& regex,
            std::function<ValueType(const std::vector<std::string>&, ArgumentTypes...)> function,
            const std::string& description = "")
        {
            if (!function)
            {
                report_empty_function( { name } );
            }
            //RegexEntry entry(name, regex, function, description);
            m_regex_entries.emplace_back(name, regex, function, description);
            return *this;
        }

        std::vector<std::string> get_names() const
        {
            std::vector<std::string> names;
            for (const auto& [name, _] : m_map)
            {
                names.push_back(name);
            }
            return names;
        }

        std::vector<std::pair<std::string, std::string>> get_descriptions() const
        {
            std::vector<std::pair<std::string, std::string>> descriptions;
            for (const auto& [name, entry] : m_map)
            {
                descriptions.push_back(std::make_pair(name, entry.description));
            }
            for (const auto& entry : m_regex_entries)
            {
                descriptions.push_back(std::make_pair(entry.name, entry.description));
            }
            return descriptions;
        }

        bool contains(const std::string& name) const
        {
            if (m_map.find(name) != m_map.end())
                return true;

            for (const auto& entry : m_regex_entries)
            {
                if (std::regex_match(name, entry.regex))
                    return true;
            }

            return false;
        }

        ValueType operator()(const std::string& name, ArgumentTypes... args) const
        try
        {
            // Try exact names first
            auto it = m_map.find(name);
            if (it != m_map.end())
                return it->second.function(args...);

            // Try provided regex (exact regex match, not substring match)
            for (const auto& entry : m_regex_entries)
            {
                std::smatch pieces_match;
                if (std::regex_match(name, pieces_match, entry.regex))
                {
                    std::vector<std::string> r_args;
                    for (size_t i = 1; i < pieces_match.size(); ++i)
                    {
                        r_args.push_back(pieces_match[i].str());
                    }
                    return entry.function(r_args, args...);
                }
            }

            // Nothing found: report error
            throw Fmi::Exception(BCP, "No function named '" + name + "'");
        }
        catch (...)
        {
            auto error = Fmi::Exception::Trace(BCP, "Exception while evaluating value");
            error.addParameter("name", name);
            throw error;
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
