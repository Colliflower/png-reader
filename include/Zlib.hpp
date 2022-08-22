#pragma once

#include <assert.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <vector>

#include "Common.hpp"

namespace trv
{
static const std::uint8_t CMFilter     = 0x0F;
static const std::uint8_t CM           = 0x08;
static const std::uint8_t CINFOFilter  = 0xF0;
static const std::uint8_t CINFOOffset  = 4;
static const std::uint8_t FDICTFilter  = 0x20;
static const std::uint8_t FLEVELFilter = 0xC0;
static const std::uint8_t BFINAL       = 0x01;
static const std::uint8_t BTYPE        = 0x06;

enum class BTYPES : std::uint8_t
{
	None        = 0x00,
	FixedHuff   = 0x01,
	DynamicHuff = 0x02,
	Err         = 0x03
};

struct DeflateArgs
{
	typedef std::vector<unsigned char> Bytes;
	bool png;
	const Bytes& input;
	Bytes& output;

	DeflateArgs(bool png, const Bytes& input, Bytes& output) :
	    png(png), input(input), output(output) {};
	DeflateArgs(bool png, const Bytes&& input, Bytes& output)  = delete;
	DeflateArgs(bool png, const Bytes& input, Bytes&& output)  = delete;
	DeflateArgs(bool png, const Bytes&& input, Bytes&& output) = delete;
};

inline constexpr std::array<uint16_t, 29 * 2> lengthExtraTable = {
	//Initial ExtraBits
	3,   0,  // 257 |  0
	4,   0,  // 258 |  1
	5,   0,  // 259	|  2
	6,   0,  // 260	|  3
	7,   0,  // 261	|  4
	8,   0,  // 262	|  5
	9,   0,  // 263	|  6
	10,  0,  // 264	|  7
	11,  1,  // 265	|  8
	13,  1,  // 266	|  9
	15,  1,  // 267	| 10
	17,  1,  // 268	| 11
	19,  2,  // 269	| 12
	23,  2,  // 270	| 13
	27,  2,  // 271	| 14
	31,  2,  // 272	| 15
	35,  3,  // 273	| 16
	43,  3,  // 274	| 17
	51,  3,  // 275	| 18
	59,  3,  // 276	| 19
	67,  4,  // 277	| 20
	83,  4,  // 278	| 21
	99,  4,  // 279	| 22
	115, 4,  // 280	| 23
	131, 5,  // 281	| 24
	163, 5,  // 282	| 25
	195, 5,  // 283	| 26
	227, 5,  // 284	| 27
	258, 0   // 285	| 28
};

inline constexpr std::array<uint16_t, 30 * 2> distanceExtraTable = {
	//Initial ExtraBits
	1,     0,   //  0
	2,     0,   //  1
	3,     0,   //  2
	4,     0,   //  3
	5,     1,   //  4
	7,     1,   //  5
	9,     2,   //  6
	13,    2,   //  7
	17,    3,   //  8
	25,    3,   //  9
	33,    4,   // 10
	49,    4,   // 11
	65,    5,   // 12
	97,    5,   // 13
	129,   6,   // 14
	193,   6,   // 15
	257,   7,   // 16
	385,   7,   // 17
	513,   8,   // 18
	769,   8,   // 19
	1025,  9,   // 20
	1537,  9,   // 21
	2049,  10,  // 22
	3073,  10,  // 23
	4097,  11,  // 24
	6145,  11,  // 25
	8193,  12,  // 26
	12289, 12,  // 27
	16385, 13,  // 28
	24577, 13   // 29
};

inline constexpr std::uint16_t FIXED_LIT_0_143_LOWER  = 0b001100000;
inline constexpr std::uint16_t FIXED_LIT_0_143_UPPER  = 0b101111111;
inline constexpr std::uint16_t FIXED_LIT_0_143_ROOT   = 0b00110000;
inline constexpr std::uint16_t FIXED_LIT_0_143_OFFSET = 0;
inline constexpr std::uint16_t FIXED_LIT_0_143_LENGTH = 8;

inline constexpr std::uint16_t FIXED_LIT_144_255_LOWER  = 0b110010000;
inline constexpr std::uint16_t FIXED_LIT_144_255_UPPER  = 0b111111111;
inline constexpr std::uint16_t FIXED_LIT_144_255_ROOT   = 0b110010000;
inline constexpr std::uint16_t FIXED_LIT_144_255_OFFSET = 144;
inline constexpr std::uint16_t FIXED_LIT_144_255_LENGTH = 9;

inline constexpr std::uint16_t FIXED_LIT_256_279_LOWER  = 0b000000000;
inline constexpr std::uint16_t FIXED_LIT_256_279_UPPER  = 0b001011100;
inline constexpr std::uint16_t FIXED_LIT_256_279_ROOT   = 0b0000000;
inline constexpr std::uint16_t FIXED_LIT_256_279_OFFSET = 256;
inline constexpr std::uint16_t FIXED_LIT_256_279_LENGTH = 7;

inline constexpr std::uint16_t FIXED_LIT_280_287_LOWER  = 0b110000000;
inline constexpr std::uint16_t FIXED_LIT_280_287_UPPER  = 0b110001111;
inline constexpr std::uint16_t FIXED_LIT_280_287_ROOT   = 0b11000000;
inline constexpr std::uint16_t FIXED_LIT_280_287_OFFSET = 280;
inline constexpr std::uint16_t FIXED_LIT_280_287_LENGTH = 8;

struct FixedHuffmanBitLengths
{
	std::uint16_t maxIndex;
	std::uint16_t bitLength;
};

template <std::integral T>
struct Huffman
{
	struct Entry
	{
		T symbol;
		std::uint16_t bitsUsed;
	};

	Huffman(uint8_t maxCodeLengthInBits, std::uint32_t symbolCount, T* symbolCodeLength) :
	    m_maxCodeLengthInBits(maxCodeLengthInBits), m_entries(1ull << maxCodeLengthInBits)
	{
		std::vector<T> codeLengthHistogram(maxCodeLengthInBits);

		for (uint32_t symbolIndex = 0; symbolIndex < symbolCount; ++symbolIndex)
		{
			assert(symbolCodeLength[symbolIndex] < maxCodeLengthInBits);
			++codeLengthHistogram[symbolCodeLength[symbolIndex]];
			assert((symbolCodeLength[symbolIndex] == 0) ||
			       (codeLengthHistogram[symbolCodeLength[symbolIndex]] <
			        (static_cast<T>(1) << symbolCodeLength[symbolIndex])));
		}

		std::vector<T> nextCode(codeLengthHistogram.size());
		codeLengthHistogram[0] = 0;

		for (uint32_t bit = 1; bit < nextCode.size(); ++bit)
		{
			nextCode[bit] = (nextCode[bit - 1] + codeLengthHistogram[bit - 1]) << 1;
		}

		for (T symbolIndex = 0; symbolIndex < symbolCount; ++symbolIndex)
		{
			T codeLengthInBits = symbolCodeLength[symbolIndex];
			// Cannot have code of length 0
			if (!codeLengthInBits) continue;

			// Pull next code for a given length, increment for next code of the same length
			T code = nextCode[codeLengthInBits]++;

			assert(code <= ((1u << (codeLengthInBits + 1u)) - 1u));

			std::uint32_t postpendBits = m_maxCodeLengthInBits - codeLengthInBits;
			for (uint32_t postpend = 0; postpend < (1u << postpendBits); ++postpend)
			{
				std::uint32_t entryIndex = (code << postpendBits) | postpend;
				Entry& entry             = m_entries[entryIndex];
				entry.symbol             = symbolIndex;

				assert(codeLengthInBits < sizeof(T) * 8);
				entry.bitsUsed = static_cast<uint16_t>(codeLengthInBits);
			}
		}
	};

	[[nodiscard]] T decode(BitConsumer<std::endian::little>& consumer)
	{
		std::uint32_t entryIndex =
		    consumer.peek_bits<uint32_t, std::endian::big>(m_maxCodeLengthInBits);
		Entry& entry = m_entries[entryIndex];

		if (!entry.bitsUsed)
		{
			throw std::runtime_error(
			    "TRV::ZLIB::DECOMPRESS encountered unpopulated entry in huffman "
			    "table.");
		}
		T result = entry.symbol;
		consumer.discard_bits(entry.bitsUsed);

		return result;
	};

   private:
	std::uint32_t m_maxCodeLengthInBits;
	std::vector<Entry> m_entries;
};

void decompress(DeflateArgs& args);
}
