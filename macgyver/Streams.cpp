#include "Streams.h"
#include <istream>
#include <ostream>
#include <boost/filesystem.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#ifndef WIN32
#include <boost/iostreams/filter/lzma.hpp>
#include <boost/iostreams/filter/zstd.hpp>
#endif

namespace fs = boost::filesystem;

enum Fmi::Compression Fmi::guess_compression_type(const std::string& theFileName)
{
    auto pos = theFileName.rfind('.');
    if (pos == std::string::npos)
        return Compression::NONE;

    const char* ext = theFileName.c_str() + pos + 1;

    if (strcasecmp(ext, "bz2") == 0)
        return Compression::BZIP2;

    if (strcasecmp(ext, "gz") == 0)
        return Compression::GZIP;
#ifndef WIN32
    if (strcasecmp(ext, "zstd") == 0)
        return Compression::ZSTD;

    if (strcasecmp(ext, "xz") == 0)
        return Compression::XZ;
#endif
    return Compression::NONE;
}

boost::optional<std::string> Fmi::lookup_file(const std::string& theFileName)
{
    boost::optional<std::string> result;

    const auto check_fn = [](const std::string& fn) -> bool
        {
            boost::system::error_code ec;
            return fs::exists(fn, ec)
                && (fs::is_regular_file(fn, ec) || fs::is_symlink(fn, ec));
        };

    if (check_fn(theFileName))
        return boost::optional<std::string>(theFileName);

    if (Fmi::guess_compression_type(theFileName) == Fmi::Compression::NONE)
    {
        static std::vector<std::string> extensions =
            {
                ".gz"
                , ".bz2"
#ifndef WIN32
                , ".xz"
                , ".zstd"
#endif
            };
        for (const auto& ext : extensions)
        {
            const std::string zfn = theFileName + ext;
            if (check_fn(zfn))
                return boost::optional<std::string>(zfn);
        }
    }

    return boost::none;
}

Fmi::IStream::IStream(std::istream& raw_input, const std::string& name)
    : boost::iostreams::filtering_stream<boost::iostreams::input>()
{
    init(raw_input, guess_compression_type(name));
}

Fmi::IStream::IStream(std::istream& raw_input, Compression compression)
{
    init(raw_input, compression);
}

void Fmi::IStream::init(std::istream& raw_input, Compression compression)
{
    switch (compression)
    {
    default:
    case Compression::NONE:
        break;

    case Compression::GZIP:
        push(boost::iostreams::gzip_decompressor());
        break;

    case Compression::BZIP2:
        push(boost::iostreams::bzip2_decompressor());
        break;

#ifndef WIN32
    case Compression::ZSTD:
        push(boost::iostreams::zstd_decompressor());
        break;

    case Compression::XZ:
        push(boost::iostreams::lzma_decompressor());
        break;
    }
#endif
    push(raw_input);
}

Fmi::OStream::OStream(std::ostream& raw_output, const std::string& name)
    : boost::iostreams::filtering_stream<boost::iostreams::output>()
{
    init(raw_output, guess_compression_type(name));
}

Fmi::OStream::OStream(std::ostream& raw_output, Compression compression)
{
    init(raw_output, compression);
}

void Fmi::OStream::init(std::ostream& raw_output, Compression compression)
{
    switch (compression)
    {
    default:
    case Compression::NONE:
        break;

    case Compression::GZIP:
        push(boost::iostreams::gzip_compressor());
        break;

    case Compression::BZIP2:
        push(boost::iostreams::bzip2_compressor());
        break;

#ifndef WIN32
    case Compression::ZSTD:
        push(boost::iostreams::zstd_compressor());
        break;

    case Compression::XZ:
        push(boost::iostreams::lzma_compressor());
        break;
    }
#endif
    push(raw_output);
}

