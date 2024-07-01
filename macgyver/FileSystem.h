#pragma once
#include <string>
#include <optional>
#include <boost/iostreams/filtering_stream.hpp>

namespace Fmi
{
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
