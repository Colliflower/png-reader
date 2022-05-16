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

		if (std::endian::native == std::endian::little)
		{
			val = std::byteswap<T>(val);
		}

		return val;
	}

	// Extract T from byte stream and advance pointer head.
	template <std::integral T>
	T extract_from_msb_bytes(char** input) {
		T val = *reinterpret_cast<T*>(*input);
		*input += sizeof(T);

		if (std::endian::native == std::endian::little)
		{
			val = std::byteswap<T>(val);
		}

		return val;
	}

	template <>
	inline char extract_from_msb_bytes<char>(char** input)
	{
		return *((*input)++);
	}

	template <>
	inline unsigned char extract_from_msb_bytes<unsigned char>(char** input)
	{
		unsigned char val = *(reinterpret_cast<unsigned char*>(*input));
		(*input)++;
		return val;
	}
	
	// Extract T from byte stream and advance pointer head.
	template <std::integral T>
	T extract_from_lsb_bytes(char** input) {
		T val = *reinterpret_cast<T*>(*input);
		*input += sizeof(T);

		if (std::endian::native == std::endian::big)
		{
			val = std::byteswap<T>(val);
		}

		return val;
	}

	template <>
	inline char extract_from_lsb_bytes<char>(char** input)
	{
		return *((*input)++);
	}

	template <>
	inline unsigned char extract_from_lsb_bytes<unsigned char>(char** input)
	{
		unsigned char val = *(reinterpret_cast<unsigned char*>(*input));
		(*input)++;
		return val;
	}

	// Swap bytes in memory if system is little endian.
	template <std::integral T>
	T big_endian(T val)
	{
		if (std::endian::native == std::endian::little)
		{
			val = std::byteswap<T>(val);
		}

		return val;
	}

	template <>
	inline char big_endian<char>(char val)
	{
		return val;
	}

	template <>
	inline unsigned char big_endian<unsigned char>(unsigned char val)
	{
		return val;
	}
	
	// Swap bytes in memory if system is big endian.
	template <std::integral T>
	T little_endian(T val)
	{
		if (std::endian::native == std::endian::big)
		{
			val = std::byteswap<T>(val);
		}

		return val;
	}

	template <>
	inline char little_endian<char>(char val)
	{
		return val;
	}

	template <>
	inline unsigned char little_endian<unsigned char>(unsigned char val)
	{
		return val;
	}
}