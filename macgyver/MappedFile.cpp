#include "MappedFile.h"
#include "Exception.h"

#ifndef _MSC_VER
#include <sys/mman.h>
#endif

Fmi::MappedFile::MappedFile(const MappedFile::Params& params)
    : boost::iostreams::mapped_file(params)
    , m_path(params.path)
{
    madvise_nodump();
}

Fmi::MappedFile::MappedFile(
    const std::string& path,
    std::ios_base::openmode mode,
    size_type length,
    boost::intmax_t offset)

    try

    : boost::iostreams::mapped_file(path, mode, length, offset)
    , m_path(path)
{
    madvise_nodump();
}
catch (...)
{
    auto exception = Fmi::Exception::Trace(BCP, "Failed to map file");
    exception.addParameter("path", path);
    throw exception;
}

Fmi::MappedFile::MappedFile(
    const std::string& path,
    mapmode mode,
    size_type length,
    boost::intmax_t offset)

    try

      : boost::iostreams::mapped_file(path, mode, length, offset)
      , m_path(path)
{
    madvise_nodump();
}
catch (...)
{
    auto exception = Fmi::Exception::Trace(BCP, "Failed to map file");
    exception.addParameter("path", path);
    exception.addParameter("mode", std::to_string(mode));
    exception.addParameter("length", std::to_string(length));
    exception.addParameter("offset", std::to_string(offset));
    throw exception;
}

Fmi::MappedFile::~MappedFile()
{
    try
    {
        madvise_default();
    }
    catch (...)
    {
        // Ignore exceptions in destructor
        return;
    }
}

void
Fmi::MappedFile::open(Params params)  try
{
    madvise_default();
    m_path = params.path;
    boost::iostreams::mapped_file::open(params);
    madvise_nodump();
}
catch (...)
{
    auto exception = Fmi::Exception::Trace(BCP, "Failed to open mapped file");
    exception.addParameter("path", params.path);
    throw exception;
}

void
Fmi::MappedFile::open(
    const std::string& path,
    std::ios_base::openmode mode,
    size_type length,
    boost::intmax_t offset ) try
{
    madvise_default();
    m_path = path;
    boost::iostreams::mapped_file::open(path, mode, length, offset);
    madvise_nodump();
}
catch (...)
{
    auto exception = Fmi::Exception::Trace(BCP, "Failed to open mapped file");
    exception.addParameter("path", path);
    exception.addParameter("mode", std::to_string(mode));
    exception.addParameter("length", std::to_string(length));
    exception.addParameter("offset", std::to_string(offset));
    throw exception;
}

void
Fmi::MappedFile::open(
    const std::string& path,
    mapmode mode,
    size_type length,
    boost::intmax_t offset) try
{
    madvise_default();
    m_path = path;
    boost::iostreams::mapped_file::open(path, mode, length, offset);
    madvise_nodump();
}
catch (...)
{
    auto exception = Fmi::Exception::Trace(BCP, "Failed to open mapped file");
    exception.addParameter("path", path);
    exception.addParameter("mode", std::to_string(mode));
    exception.addParameter("length", std::to_string(length));
    exception.addParameter("offset", std::to_string(offset));
    throw exception;
}

void
Fmi::MappedFile::close() try
{
    madvise_default();
    boost::iostreams::mapped_file::close();
}
catch (...)
{
    auto exception = Fmi::Exception::Trace(BCP, "Failed to close mapped file");
    exception.addParameter("Path", m_path);
    throw exception;
}

void
Fmi::MappedFile::madvise_nodump()
{
    if (boost::iostreams::mapped_file::is_open())
    {
        invoke_madvise(MADV_DONTDUMP);
    }
}

void
Fmi::MappedFile::madvise_default()
{
    if (boost::iostreams::mapped_file::is_open())
    {
        invoke_madvise(MADV_NORMAL);
    }
}

void
Fmi::MappedFile::invoke_madvise(int adv)
{
#ifndef _MSC_VER
    char *address = boost::iostreams::mapped_file::data()
        ? boost::iostreams::mapped_file::data()
        : const_cast<char*>(boost::iostreams::mapped_file::const_data());

    int ret_val = madvise(address, size(), adv);
    if (ret_val < 0)
    {
        // FIXME: do we want to throw exception here?
        //        Maybe we should simply ignore the error.
        const int err = errno;
        std::array<char,80> tmp{};
        Fmi::Exception exception(BCP, "madvise() failed");
        exception.addParameter("errno", std::to_string(err));
        exception.addParameter("description", strerror_r(err, tmp.data(), tmp.size()));
        static_cast<void>(snprintf(tmp.data(), tmp.size(), "%p", (void*)address));
        exception.addParameter("address", tmp.data());
        exception.addParameter("size", std::to_string(size()));
        throw exception;
    }
#else
    (void)adv;
#endif
}

