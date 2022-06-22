#pragma once

#include <array>
#include <concepts>

#include "Common.hpp"

namespace trv
{
struct CRCTable
{
   public:
	constexpr CRCTable() : lookupTable()
	{
		for (size_t byte = 0; byte < 256; ++byte)
		{
			std::uint32_t c = static_cast<uint32_t>(byte);
			for (int bit = 0; bit < 8; ++bit)
			{
				if (c & 1)
				{
					c = 0xedb88320UL ^ (c >> 1);
				}
				else
				{
					c >>= 1;
				}
			}

			lookupTable.at(byte) = c;
		}
	}

	[[nodiscard]] std::uint32_t crc(const void* buf, std::size_t len) const
	{
		return update_crc(0xFFFFFFFFUL, buf, len) ^ 0xFFFFFFFFUL;
	}

	[[nodiscard]] std::uint32_t crc(uint32_t crc, const void* buf, std::size_t len) const
	{
		return update_crc(crc ^ 0xFFFFFFFFUL, buf, len) ^ 0xFFFFFFFFUL;
	}

   private:
	[[nodiscard]] std::uint32_t update_crc(uint32_t crc, const void* buf, std::size_t len) const
	{
		std::uint32_t c = crc;

		for (size_t i = 0; i < len; ++i)
		{
			std::uint8_t curr = *(static_cast<const std::uint8_t*>(buf) + i);
			c                 = lookupTable[(c ^ curr) & 0xFF] ^ (c >> 8);
		}

		return c;
	}

	std::array<uint32_t, 256> lookupTable;
};
}
