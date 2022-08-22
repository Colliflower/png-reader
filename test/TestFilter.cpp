#include <gtest/gtest.h>

#include <limits>

#ifndef TRV_TEST_MULTITHREADED
#undef TRV_PNG_MULTITHREADED
#endif

#include "Filter.hpp"

struct TestIHDR : public trv::IHDR
{
	TestIHDR(std::uint32_t width, std::uint32_t height, std::uint8_t bitDepth,
	         std::uint8_t colorType, std::uint8_t compressionMethod, std::uint8_t filterMethod,
	         std::uint8_t interlaceMethod) :
	    trv::IHDR()
	{
		this->width             = width;
		this->height            = height;
		this->bitDepth          = bitDepth;
		this->colorType         = colorType;
		this->compressionMethod = compressionMethod;
		this->filterMethod      = filterMethod;
		this->interlaceMethod   = interlaceMethod;
	};
};

struct TestPLTE : public trv::PLTE
{
	TestPLTE(const std::vector<unsigned char>& data) : PLTE() { this->data = data; };
};

TEST(TestFilter, TestNoFilterU8)
{
	std::vector<std::uint8_t> output;
	std::vector<unsigned char> input { 0, 255, 255, 255 };

	TestIHDR header { 1, 1, 8, 2, 0, 0, 0 };

	trv::FilterArgs<std::uint8_t> args { input, &header, nullptr, output };

	trv::unfilter(args);

	EXPECT_EQ(output.size(), 3);

	for (const auto& val : output)
	{
		EXPECT_EQ(val, std::numeric_limits<std::uint8_t>::max());
	}
}

TEST(TestFilter, TestNoFilterU16)
{
	std::vector<std::uint16_t> output;
	std::vector<unsigned char> input { 0, 255, 255, 255 };

	TestIHDR header { 1, 1, 8, 2, 0, 0, 0 };

	trv::FilterArgs<std::uint16_t> args { input, &header, nullptr, output };

	trv::unfilter(args);

	EXPECT_EQ(output.size(), 3);

	for (const auto& val : output)
	{
		EXPECT_EQ(val, std::numeric_limits<std::uint16_t>::max());
	}
}

TEST(TestFilter, TestNoFilterU32)
{
	std::vector<std::uint32_t> output;
	std::vector<unsigned char> input { 0, 255, 255, 255 };

	TestIHDR header { 1, 1, 8, 2, 0, 0, 0 };

	trv::FilterArgs<std::uint32_t> args { input, &header, nullptr, output };

	trv::unfilter(args);

	EXPECT_EQ(output.size(), 3);

	for (const auto& val : output)
	{
		EXPECT_EQ(val, std::numeric_limits<std::uint32_t>::max());
	}
}

TEST(TestFilter, TestNoFilterI16)
{
	std::vector<std::int16_t> output;
	std::vector<unsigned char> input { 0, 255, 255, 255 };

	TestIHDR header { 1, 1, 8, 2, 0, 0, 0 };

	trv::FilterArgs<std::int16_t> args { input, &header, nullptr, output };

	trv::unfilter(args);

	EXPECT_EQ(output.size(), 3);

	for (const auto& val : output)
	{
		EXPECT_EQ(val, std::numeric_limits<std::int16_t>::max());
	}
}

TEST(TestFilter, TestNoFilterI32)
{
	std::vector<std::int32_t> output;
	std::vector<unsigned char> input { 0, 255, 255, 255 };

	TestIHDR header { 1, 1, 8, 2, 0, 0, 0 };

	trv::FilterArgs<std::int32_t> args { input, &header, nullptr, output };

	trv::unfilter(args);

	EXPECT_EQ(output.size(), 3);

	for (const auto& val : output)
	{
		EXPECT_EQ(val, std::numeric_limits<std::int32_t>::max());
	}
}
