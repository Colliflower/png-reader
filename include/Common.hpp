#pragma once

#include <concepts>
#include <fstream>
#include <bit>
#include <vector>
#include <assert.h>

namespace trv
{
	enum class InterlaceMethod : std::uint8_t { None, Adam7 };
	enum class FilterMethod : std::uint8_t { None, Sub, Up, Average, Paeth };
	enum class ColorType : std::uint8_t { Palette = 0b001, Color = 0b010, Alpha = 0b100 };

	inline std::uint8_t reverse_byte(uint8_t val)
	{
		return static_cast<uint8_t>(((val * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32);
	}

	template <std::unsigned_integral T>
	inline T reverse_bits(T val)
	{
		T result = 0;

		for (int byte = 0; byte < sizeof(T); ++byte)
		{
			result |= reverse_byte(static_cast<uint8_t>((val >> (byte * 8)) & 0xFF)) << ((sizeof(T) - byte - 1) * 8);
		}

		return result;
	}

	template <>
	inline std::uint8_t reverse_bits<uint8_t>(uint8_t val)
	{
		return reverse_byte(val);
	}

	// Allows reading of integral types from big-endian binary stream
	template <std::integral T>
	T extract_from_ifstream(std::basic_ifstream<char>& input) {
		char bytes[sizeof(T)];
		input.read(bytes, sizeof(T));
		T val;
		std::memcpy(&val, bytes, sizeof(T));


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
	inline std::uint8_t big_endian<uint8_t>(uint8_t val)
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
	inline std::uint8_t little_endian<uint8_t>(uint8_t val)
	{
		return val;
	}

	template<std::endian inputByteType>
	class BitConsumer
	{
	public:
		BitConsumer(const std::vector<unsigned char>& input) :
			m_input(input)
		{
		};

		template<std::endian Other>
		BitConsumer(const BitConsumer<Other>& other) :
			m_input(other.m_input),
			m_bitsConsumed(other.m_bitsConsumed),
			m_bytesConsumed(other.m_bytesConsumed)
		{
		};

		template<std::integral T, std::endian outputBitType>
		T peek_bits(size_t bits)
		{
			assert(m_bitsConsumed < 8);

			if (!bits)
			{
				return 0;
			}

			std::uint64_t result = 0;
			std::uint64_t val = 0;

			std::size_t bytesRequested = ((bits + m_bitsConsumed + 7) / 8) - 1;

			for (size_t byte = 0; byte <= bytesRequested; ++byte)
			{
				if constexpr (inputByteType == std::endian::little)
				{
					val |= static_cast<uint64_t>(m_input[m_bytesConsumed + byte]) << (byte * 8);
				}
				else
				{
					val |= static_cast<uint64_t>(m_input[m_bytesConsumed + byte]) << ((bytesRequested - byte) * 8);
				}
			}

			if constexpr (inputByteType == std::endian::little)
			{
				val = val >> m_bitsConsumed;
			}
			else
			{
				// mask off previously used bits
				val &= (1ull << static_cast<uint64_t>((bytesRequested + 1ull) * 8ull - m_bitsConsumed)) - 1ull;
				// shift away bits that will be unused
				val = val >> (((bytesRequested + 1ull) * 8ull) - bits - m_bitsConsumed);
			}

			if constexpr (inputByteType != outputBitType)
			{
				for (uint8_t byte = 0; byte < ((bits + 7) / 8); ++byte)
				{
					std::uint8_t curr = (val >> (byte * 8)) & 0xFFu;

					curr = reverse_bits(curr);

					result |= static_cast<uint64_t>(curr) << (64ull - ((byte + 1ull) * 8ull));
				}

				result = result >> (64ull - bits);
			}
			else
			{
				result = val & ((1ull << bits) - 1ull);
			}

			return static_cast<T>(result);
		};

		void discard_bits(size_t bits)
		{
			m_bytesConsumed += (m_bitsConsumed + bits) / 8;
			m_bitsConsumed = (m_bitsConsumed + bits) % 8;
		};

		void flush_byte()
		{
			m_bytesConsumed++;
			m_bitsConsumed = 0;
		}

		template <typename T, std::endian outputBitType>
		T consume_bits(size_t bits)
		{
			T result = peek_bits<T, outputBitType>(bits);
			discard_bits(bits);
			return result;
		}

	private:
		std::uint8_t m_bitsConsumed = 0;
		std::size_t m_bytesConsumed = 0;
		const std::vector<unsigned char>& m_input;

		template <std::endian otherBitType>
		friend class BitConsumer;
	};
}