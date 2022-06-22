#include "Filter.hpp"

#include "Image.hpp"

namespace trv
{
[[nodiscard]] static std::uint8_t paethPredictor(uint8_t left,
                                                 std::uint8_t top,
                                                 std::uint8_t topleft)
{
	std::int32_t p        = static_cast<int32_t>(left + top - topleft);
	std::int32_t pleft    = p > left ? p - left : left - p;
	std::int32_t ptop     = p > top ? p - top : top - p;
	std::int32_t ptopleft = p > topleft ? p - topleft : topleft - p;

	assert(pleft >= 0 && ptop >= 0 && ptopleft >= 0);

	if (pleft <= ptop && pleft <= ptopleft)
	{
		return left;
	}
	else if (ptop <= ptopleft)
	{
		return top;
	}
	else
	{
		return topleft;
	}
};

void do_unfilter(std::vector<uint8_t>& input,
                 std::size_t offset,
                 std::size_t scanlines,
                 std::size_t byteWidth,
                 std::size_t bpp)
{
	for (size_t scanline = 0; scanline < scanlines; ++scanline)
	{
		FilterMethod filterType = static_cast<FilterMethod>(input[scanline * byteWidth + offset]);

		if (filterType == FilterMethod::None || (filterType == FilterMethod::Up && scanline == 0))
		{
			continue;
		}

		for (size_t byte = 1; byte < byteWidth; ++byte)
		{
			std::uint8_t value = 0;
			switch (static_cast<FilterMethod>(filterType))
			{
				case FilterMethod::Sub:
					{
						if (byte <= bpp) continue;
						std::uint8_t left = input[scanline * byteWidth + byte - bpp + offset];
						value             = left;
						break;
					}
				case FilterMethod::Up:
					{
						std::uint8_t top = input[(scanline - 1) * byteWidth + byte + offset];
						value            = top;
						break;
					}
				case FilterMethod::Average:
					{
						std::uint8_t top  = 0;
						std::uint8_t left = 0;

						if (scanline != 0) top = input[(scanline - 1) * byteWidth + byte + offset];

						if (byte > bpp) left = input[scanline * byteWidth + byte - bpp + offset];

						value = static_cast<uint8_t>(static_cast<uint16_t>(top + left) >> 1);
						break;
					}
				case FilterMethod::Paeth:
					{
						std::uint8_t top     = 0;
						std::uint8_t left    = 0;
						std::uint8_t topleft = 0;

						if (scanline != 0 && byte > bpp)
							topleft = input[(scanline - 1) * byteWidth + byte - bpp + offset];

						if (scanline != 0) top = input[(scanline - 1) * byteWidth + byte + offset];

						if (byte > bpp) left = input[scanline * byteWidth + byte - bpp + offset];

						value = paethPredictor(left, top, topleft);
						break;
					}
				default:
					throw std::runtime_error(
					    "TRV::IMAGE::LOAD_IMAGE Encountered unexpected filter "
					    "type.");
					break;
			}

			input[scanline * byteWidth + byte + offset] += value;
		}
	}
}
}
