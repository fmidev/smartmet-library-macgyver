// ======================================================================
/*!
 * \file
 * \brief Regression tests for Streams.h
 */
// ======================================================================

#include "Streams.h"
#include "Exception.h"
#include <regression/tframe.h>
#include <sstream>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/process.hpp>
#include <stdio.h>

namespace io = boost::iostreams;
namespace p = boost::process;

std::string read_orig_file()
{
    std::string result;
    std::ifstream test_input("StreamsTest.cpp");
    io::copy(test_input, std::back_inserter(result));
    return result;
}

void read_gzip_compressed_stream()
{
    std::string s1, s2 = read_orig_file();

    boost::filesystem::create_directory("tmp");
    system("cat StreamsTest.cpp | gzip >tmp/StreamsTest.cpp.gz");
    std::ifstream pipe("tmp/StreamsTest.cpp.gz");
    Fmi::IStream input(pipe, "tmp/StreamsTest.cpp.gz");
    io::copy(input, std::back_inserter(s1));

    if (s1 != s2)
        TEST_FAILED("Failed to decompress GZIP compressed file");

    TEST_PASSED();
}

void read_xz_compressed_stream()
{
    std::string s1, s2 = read_orig_file();

    boost::filesystem::create_directory("tmp");
    system("cat StreamsTest.cpp | xz >tmp/StreamsTest.cpp.xz");
    std::ifstream pipe("tmp/StreamsTest.cpp.xz");
    Fmi::IStream input(pipe, Fmi::Compression::XZ);
    io::copy(input, std::back_inserter(s1));

    if (s1 != s2)
    {
        TEST_FAILED("Failed to decompress XZ compressed file");
    }
    TEST_PASSED();
}

std::string compress(const std::string& src, Fmi::Compression compression)
{
    std::istringstream input(src);
    std::ostringstream raw_output;
    Fmi::OStream output(raw_output, compression);
    io::copy(input, output);
    return raw_output.str();
}

std::string decompress(const std::string& src, Fmi::Compression compression)
{
    std::istringstream raw_input(src);
    std::ostringstream output;
    Fmi::IStream input(raw_input, compression);
    io::copy(input, output);
    return output.str();
}

void test_xz()
{
    std::string s0 = read_orig_file();
    std::string s1 = compress(s0, Fmi::Compression::XZ);
    std::string s2 = decompress(s1, Fmi::Compression::XZ);

    if (s0 != s2)
        TEST_FAILED("XZ compression/decompression test failed");

    TEST_PASSED();
}

void test_zstd()
{
    std::string s0 = read_orig_file();
    std::string s1 = compress(s0, Fmi::Compression::ZSTD);
    std::string s2 = decompress(s1, Fmi::Compression::ZSTD);

    if (s0 != s2)
        TEST_FAILED("ZSTD compression/decompression test failed");

    TEST_PASSED();
}

void test_bzip2()
{
    std::string s0 = read_orig_file();
    std::string s1 = compress(s0, Fmi::Compression::BZIP2);
    std::string s2 = decompress(s1, Fmi::Compression::BZIP2);

    if (s0 != s2)
        TEST_FAILED("BZIP2 compression/decompression test failed");

    TEST_PASSED();
}

void test_gzip()
{
    std::string s0 = read_orig_file();
    std::string s1 = compress(s0, Fmi::Compression::GZIP);
    std::string s2 = decompress(s1, Fmi::Compression::GZIP);

    if (s0 != s2)
        TEST_FAILED("ZSTD compression/decompression test failed");

    TEST_PASSED();
}

void test_none()
{
    std::string s0 = read_orig_file();
    std::string s1 = compress(s0, Fmi::Compression::NONE);
    std::string s2 = decompress(s1, Fmi::Compression::NONE);

    if (s0 != s1)
        TEST_FAILED("No compression/decompression test failed");

    if (s0 != s2)
        TEST_FAILED("No compression/decompression test failed");

    TEST_PASSED();
}

// ----------------------------------------------------------------------
/*!
 * The actual test suite
 */
// ----------------------------------------------------------------------

class tests : public tframe::tests
{
  virtual const char* error_message_prefix() const { return "\n\t"; }

  void test(void)
  {
    TEST(read_gzip_compressed_stream);
    TEST(read_xz_compressed_stream);
    TEST(test_xz);
    TEST(test_zstd);
    TEST(test_bzip2);
    TEST(test_gzip);
    TEST(test_none);
  }
};

int main(void)
{
  std::cout << std::endl << "Compressed streams tester" << std::endl << "=====================" << std::endl;
  tests t;
  return t.run();
}
