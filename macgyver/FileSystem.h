#pragma once

#include <string>
#include <optional>
#include <filesystem>
#include <boost/iostreams/filtering_stream.hpp>

namespace Fmi
{
    /**
     *   Replacement of boost::filesystem::last_write_time
     *
     *   Return type changes in std::filesystem. This method returns std::time_t
     *   similarly as boost::filesystem method
     *
     *   @param path Path to the file
     *   @param ec Error code
     *   @return Last write time of the file or 0 in case of an error
     *
     *   Does not th
     */
    std::time_t last_write_time(const std::filesystem::path& path, std::error_code& ec) throw();

    /**
     *   Replacement of boost::filesystem::last_write_time
     *
     *   Return type changes in std::filesystem. This method returns std::time_t
     *   similarly as boost::filesystem method
     *
     *   @param path Path to the file
     *   @return Last write time of the file
     *
     *   Throws an exception in case of an error
     */
    std::time_t last_write_time(const std::filesystem::path& path);

    /**
     *   Returns file last write time or provided default time in case of an error
     */
    std::time_t last_write_time_or(const std::filesystem::path& path, const std::time_t default_time) throw();

    /**
     *   Replacement of boost::filesystem::unique_path (not found in std::filesystem)
     *
     *   Implementation generated by ChatGPT 4o (with some small additional changes)
     */
    std::filesystem::path unique_path(const std::filesystem::path& model = "%%%%-%%%%-%%%%-%%%%");

    enum Compression
    {
        NONE
        ,BZIP2
        ,GZIP
#ifndef WIN32
        ,XZ
        ,ZSTD
#endif
    };

    enum Compression guess_compression_type(const std::string& theFileName);

    inline bool is_compressed(const std::string& theFileName)
    {
        return guess_compression_type(theFileName) != Compression::NONE;
    }

    /**
     *   Lookup file (including compressed ones)
     */
    std::optional<std::string> lookup_file(const std::string& theFileName);

    class IStream : public boost::iostreams::filtering_stream<boost::iostreams::input>
    {
    public:
        IStream(std::istream& raw_input, const std::string& name);
        IStream(std::istream& raw_input, Compression compression);
        virtual ~IStream() = default;

    private:
        void init(std::istream& raw_input, Compression compression);
    };

    class OStream : public boost::iostreams::filtering_stream<boost::iostreams::output>
    {
    public:
        OStream(std::ostream& raw_output, const std::string& name);
        OStream(std::ostream& raw_output, Compression compression);
        virtual ~OStream() = default;

    private:
        void init(std::ostream& raw_output, Compression compression);
    };
}
