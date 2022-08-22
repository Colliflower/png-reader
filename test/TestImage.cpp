
#include <gtest/gtest.h>

#include <iostream>
#include <string>
#include <vector>

#ifndef TRV_TEST_MULTITHREADED
#undef TRV_PNG_MULTITHREADED
#endif

#include "Image.hpp"

TEST(TestImage, TestLoadImages)
{
#ifdef TRV_PNG_MULTITHREADED
	std::cout << "NOOO\n";
#endif
	static const std::vector<std::string> files = { "plte_bit_depth_1.png" };

	for (const auto& file : files)
	{
		const std::string path { "./samples/" + file };

		trv::Image<std::uint8_t> img { trv::load_image<std::uint8_t>(path) };
	}
}
