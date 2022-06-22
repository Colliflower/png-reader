#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "Image.hpp"

TEST(TestImage, TestLoadImages)
{
	static const std::vector<std::string> files = { "plte_bit_depth_1.png" };

	for (const auto& file : files)
	{
		const std::string path { "./samples/" + file };

		trv::Image<std::uint8_t> img { trv::load_image<std::uint8_t>(path) };
	}
}