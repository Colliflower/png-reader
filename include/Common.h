#pragma once

#include <concepts>
#include <fstream>
#include <bit>

namespace trv
{
	// Allows reading of integral types from big-endian binary stream
	template <std::integral T>
	T extract_from_ifstream(std::basic_ifstream<char>& input) {
		T val;
		input.read(reinterpret_cast<char*>(&val), sizeof(val));

		if constexpr (std::endian::native == std::endian::little)
		{
			val = std::byteswap<T>(val);
		}

		return val;
	}

	// Extract T from byte stream and advance pointer head.
	template <std::integral T>
	T extract_from_msb_bytes(uint8_t** input) {
		T val = *reinterpret_cast<T*>(*input);
		*input += sizeof(T);

		if constexpr (std::endian::native == std::endian::little)
		{
			val = std::byteswap<T>(val);
		}

		return val;
	}

	template <>
	inline uint8_t extract_from_msb_bytes<uint8_t>(uint8_t** input)
	{
		return *((*input)++);
	}
	
	// Extract T from byte stream and advance pointer head.
	template <std::integral T>
	T extract_from_lsb_bytes(uint8_t** input) {
		T val = *reinterpret_cast<T*>(*input);
		*input += sizeof(T);

		if (std::endian::native == std::endian::big)
		{
			val = std::byteswap<T>(val);
		}

		return val;
	}

	template <>
	inline uint8_t extract_from_lsb_bytes<uint8_t>(uint8_t** input)
	{
		return *((*input)++);
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