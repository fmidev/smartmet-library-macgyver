#pragma once

#include <boost/iostreams/device/mapped_file.hpp>

namespace Fmi
{
    class MappedFile : private boost::iostreams::mapped_file
    {
    public:

        using size_type = boost::iostreams::mapped_file::size_type;
        static constexpr std::size_t max_length = static_cast<size_type>(-1);
        using Params = boost::iostreams::mapped_file_params;
        using boost::iostreams::mapped_file::mapmode;
        using boost::iostreams::mapped_file::iterator;
        using boost::iostreams::mapped_file::const_iterator;

        MappedFile(const MappedFile&) = delete;
        MappedFile(MappedFile&&) = delete;
        MappedFile& operator = (const MappedFile&) = delete;
        MappedFile& operator = (MappedFile&&) = delete;

        explicit MappedFile() = default;

        explicit MappedFile(const Params& params);

        explicit MappedFile(
            const std::string& path,
            std::ios_base::openmode mode = std::ios_base::in /* | std::ios_base::out */ ,
            size_type length = max_length,
            boost::intmax_t offset = 0 );

        explicit MappedFile(
            const std::string& path,
            mapmode mode,
            size_type length = max_length,
            boost::intmax_t offset = 0 );

         virtual ~MappedFile();

        void open(const Params& params);

        void open(
            const std::string& path,
            std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out,
            size_type length = max_length,
            boost::intmax_t offset = 0 );

        void open(
            const std::string& path,
            mapmode mode,
            size_type length = max_length,
            boost::intmax_t offset = 0 );

        bool is_open() const { return boost::iostreams::mapped_file::is_open(); }

        mapmode flags() const { return boost::iostreams::mapped_file::flags(); }

        void close();

        size_type size() const { return boost::iostreams::mapped_file::size(); }

        char* data() const { return boost::iostreams::mapped_file::data(); }

        const char* const_data() const { return boost::iostreams::mapped_file::const_data(); }

        iterator begin() const { return boost::iostreams::mapped_file::begin(); }

        const_iterator const_begin() const { return boost::iostreams::mapped_file::const_begin(); }

        iterator end() const { return boost::iostreams::mapped_file::end(); }

        const_iterator const_end() const { return boost::iostreams::mapped_file::const_end(); }

        static int alignment() { return boost::iostreams::mapped_file::alignment(); }

    private:
        std::string m_path;
        void madvise_nodump();
        void madvise_default();
        void invoke_madvise(int adv);

    };
}
