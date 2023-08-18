#include "MappedFile.h"
#include <boost/test/included/unit_test.hpp>
#include <cmath>
#include <cstring>
#include "Exception.h"

using namespace boost::unit_test;

namespace
{
    struct Cleanup
    {
        Cleanup(const std::string& fn) : fn(fn) {}
        ~Cleanup() { unlink(fn.c_str()); }
        const std::string fn;
    };
}

test_suite* init_unit_test_suite(int argc, char* argv[])
{
  const char* name = "MappedFile tester";
  unit_test_log.set_threshold_level(log_messages);
  framework::master_test_suite().p_name.value = name;
  BOOST_TEST_MESSAGE("");
  BOOST_TEST_MESSAGE(name);
  BOOST_TEST_MESSAGE(std::string(std::strlen(name), '='));
  return NULL;
}

BOOST_AUTO_TEST_CASE(test_mapping_file_ro)
{
    try {
        Fmi::MappedFile mapped_file;
        mapped_file.open("MappedFileTest.cpp");
        const char* data = mapped_file.const_data();
        BOOST_REQUIRE(data != nullptr);
        const std::size_t size = mapped_file.size();
        std::string content(data, data + size);
        BOOST_REQUIRE(content.find("MappedFileTest.cpp") != std::string::npos);
    } catch (Fmi::Exception& e) {
        std::cout << e << std::endl;
        BOOST_REQUIRE(false);
    }
}

BOOST_AUTO_TEST_CASE(test_mapping_file_rw)
{
    try {
        constexpr std::size_t size = 1024;

        char buffer[size];
        for (std::size_t i = 0; i < size; i++)
        {
            buffer[i] = i % 256U;
        }

        const std::string fn = "MappedFile.tmp";
        Cleanup cleanup(fn);
        Fmi::MappedFile::Params params(fn);
        params.flags = Fmi::MappedFile::mapmode::readwrite;
        params.new_file_size = size;

        Fmi::MappedFile mapped_file;
        mapped_file.open(params);
        char* data = mapped_file.data();
        BOOST_REQUIRE(data != nullptr);
        std::memcpy(buffer, data, size);
        mapped_file.close();

        std::ostringstream tmp;
        std::ifstream input(fn);
        std::copy(
            std::istreambuf_iterator<char>(input.rdbuf()),
            std::istreambuf_iterator<char>(),
            std::ostream_iterator<char>(tmp));
        BOOST_CHECK_EQUAL(tmp.str().length(), size);
        BOOST_REQUIRE(memcmp(buffer, tmp.str().c_str(), size) == 0);
    } catch (Fmi::Exception& e) {
        std::cout << e << std::endl;
        BOOST_REQUIRE(false);
    }
}
