#pragma once

#include <concepts>
#include <fstream>
#include <bit>

namespace trv
{
	enum class InterlaceMethod : uint8_t { None, Adam7 };
	enum class FilterMethod : uint8_t { None, Sub, Up, Average, Paeth };
	enum class ColorType : uint8_t { Palette = 0b001, Color = 0b010, Alpha = 0b100 };

	// Allows reading of integral types from big-endian binary stream
	template <std::integral T>
	T extract_from_ifstream(std::basic_ifstream<char>& input) {
		T val;
		input.read(reinterpret_cast<char*>(&val), sizeof(val));

		if constexpr (std::endian::native == std::endian::little && sizeof(T) > 1)
		{
			val = std::byteswap<T>(val);
		}

		return val;
	}

	// Swap bytes in memory if system is little endian.
	template <std::integral T>
	T big_endian(T val)
	{
		if constexpr (std::endian::native == std::endian::little)
		{
			val = std::byteswap<T>(val);
		}

		return val;
	}

	template <>
	inline uint8_t big_endian<uint8_t>(uint8_t val)
	{
		return val;
	}
	
	// Swap bytes in memory if system is big endian.
	template <std::integral T>
	T little_endian(T val)
	{
		if constexpr (std::endian::native == std::endian::big)
		{
			val = std::byteswap<T>(val);
		}

		return val;
	}

	template <>
	inline uint8_t little_endian<uint8_t>(uint8_t val)
	{
		return val;
	}
}