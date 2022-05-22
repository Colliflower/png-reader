#pragma once

#include <stdint.h>
#include "Common.h"
#include <algorithm>
#include <iterator>
#include <vector>
#include <array>
#include <assert.h>

namespace trv
{
	static const uint8_t CMFilter = 0x0F;
	static const uint8_t CM = 0x08;
	static const uint8_t CINFOFilter = 0xF0;
	static const uint8_t CINFOOffset = 4;
	static const uint8_t FDICTFilter = 0x20;
	static const uint8_t FLEVELFilter = 0xC0;
	static const uint8_t BFINAL = 0x01;
	static const uint8_t BTYPE = 0x06;

	enum class BTYPES : uint8_t { None = 0x00, FixedHuff = 0x01, DynamicHuff=0x02, Err=0x03 };

	struct DeflateArgs
	{
		bool png;
		const std::vector<uint8_t>& input;
		std::vector<uint8_t>& output;
	};

	template<std::endian inputByteType>
	class BitConsumer
	{
	public:
		BitConsumer(const std::vector<uint8_t>& input) :
			input(input)
		{
		};
		
		template<std::endian Other>
		BitConsumer(const BitConsumer<Other>& other) :
			input(other.input),
			bitsConsumed(other.bitsConsumed),
			bytesConsumed(other.bytesConsumed)
		{
		};

		template<std::integral T, std::endian outputBitType>
		T peek_bits(size_t bits)
		{
			assert(bitsConsumed < 8);

			if (!bits)
			{
				return 0;
			}

			uint64_t result = 0;
			uint64_t val = 0;

			size_t bytesRequested = ((bits + bitsConsumed + 7) / 8) - 1;

			for (size_t byte = 0; byte <= bytesRequested; ++byte)
			{
				if constexpr (inputByteType == std::endian::little)
				{
					val |= static_cast<uint64_t>(input[bytesConsumed + byte]) << (byte * 8);
				}
				else
				{
					val |= static_cast<uint64_t>(input[bytesConsumed + byte]) << ((bytesRequested - byte) * 8);
				}
			}

			if constexpr (inputByteType == std::endian::little)
			{
				val = val >> bitsConsumed;
			}
			else
			{
				// mask off previously used bits
				val &= (1ull << static_cast<uint64_t>((bytesRequested + 1ull) * 8ull - bitsConsumed)) - 1ull;
				// shift away bits that will be unused
				val = val >> (((bytesRequested + 1ull)* 8ull) - bits - bitsConsumed);
			}

			if constexpr (inputByteType != outputBitType)
			{
				for (uint8_t byte = 0; byte < ((bits + 7) / 8); ++byte)
				{
					uint8_t curr = (val >> (byte * 8)) & 0xFFu;
					
					curr = static_cast<uint8_t>(((curr * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32);

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
			bytesConsumed += (bitsConsumed + bits) / 8;
			bitsConsumed = (bitsConsumed + bits) % 8;
		};

		void flush_byte()
		{
			bytesConsumed++;
			bitsConsumed = 0;
		}

		template <typename T, std::endian outputBitType>
		T consume_bits(size_t bits)
		{
			T result = peek_bits<T, outputBitType>(bits);
			discard_bits(bits);
			return result;
		}

		uint8_t bitsConsumed = 0;
		size_t bytesConsumed = 0;
		const std::vector<uint8_t>& input;
	};

	static const std::array<uint16_t, 29 * 2> lengthExtraTable =
	{
		//Initial ExtraBits
			  3,      0, // 257 |  0
			  4,      0, // 258 |  1
			  5,      0, // 259	|  2
			  6,      0, // 260	|  3
			  7,      0, // 261	|  4
			  8,      0, // 262	|  5
			  9,      0, // 263	|  6
			 10,      0, // 264	|  7
			 11,      1, // 265	|  8
			 13,      1, // 266	|  9
			 15,      1, // 267	| 10
			 17,      1, // 268	| 11
			 19,      2, // 269	| 12
			 23,      2, // 270	| 13
			 27,      2, // 271	| 14
			 31,      2, // 272	| 15
			 35,      3, // 273	| 16
			 43,      3, // 274	| 17
			 51,      3, // 275	| 18
			 59,      3, // 276	| 19
			 67,      4, // 277	| 20
			 83,      4, // 278	| 21
			 99,      4, // 279	| 22
			115,      4, // 280	| 23
			131,      5, // 281	| 24
			163,      5, // 282	| 25
			195,      5, // 283	| 26
			227,      5, // 284	| 27
			258,      0	 // 285	| 28
	};

	static const std::array<uint16_t, 30 * 2> distanceExtraTable =
	{
    //Initial ExtraBits
			1,    0,  //  0
			2,    0,  //  1
			3,    0,  //  2
			4,    0,  //  3
			5,    1,  //  4
			7,    1,  //  5
			9,    2,  //  6
		   13,    2,  //  7
		   17,    3,  //  8
		   25,    3,  //  9
		   33,    4,  // 10
		   49,    4,  // 11
		   65,    5,  // 12
		   97,    5,  // 13
		  129,    6,  // 14
		  193,    6,  // 15
		  257,    7,  // 16
		  385,    7,  // 17
		  513,    8,  // 18
		  769,    8,  // 19
		 1025,    9,  // 20
		 1537,    9,  // 21
		 2049,   10,  // 22
		 3073,   10,  // 23
		 4097,   11,  // 24
		 6145,   11,  // 25
		 8193,   12,  // 26
		12289,   12,  // 27
		16385,   13,  // 28
		24577,   13   // 29
	};

	template <std::integral T>
	struct Huffman
	{
		struct Entry
		{
			T symbol;
			uint16_t bitsUsed;
		};

		Huffman(uint8_t maxCodeLengthInBits, uint32_t symbolCount, T* symbolCodeLength, T symbolAddend = 0) :
			maxCodeLengthInBits(maxCodeLengthInBits),
			entries(1ull << maxCodeLengthInBits)
		{
 			std::vector<T> codeLengthHistogram(maxCodeLengthInBits);

			for (uint32_t symbolIndex = 0; symbolIndex < symbolCount; ++symbolIndex)
			{
				assert(symbolCodeLength[symbolIndex] < maxCodeLengthInBits);
				++codeLengthHistogram[symbolCodeLength[symbolIndex]];
				assert((symbolCodeLength[symbolIndex] == 0) || (codeLengthHistogram[symbolCodeLength[symbolIndex]] < (1 << symbolCodeLength[symbolIndex])));
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
				if (!codeLengthInBits)
					continue;

				// Pull next code for a given length, increment for next code of the same length
				T code = nextCode[codeLengthInBits]++;

				assert(code <= ((1u << (codeLengthInBits + 1u)) - 1u));

				uint32_t postpendBits = maxCodeLengthInBits - codeLengthInBits;
				for (uint32_t postpend = 0; postpend < (1u << postpendBits); ++postpend)
				{
					uint32_t entryIndex = (code << postpendBits) | postpend;
					Entry& entry = entries[entryIndex];
					entry.symbol = symbolIndex + symbolAddend;

					assert(codeLengthInBits < sizeof(T) * 8);
					entry.bitsUsed = static_cast<uint16_t>(codeLengthInBits);
				}
			}
		};

		T decode(BitConsumer <std::endian::little>& consumer)
		{	  
			uint32_t entryIndex = consumer.peek_bits<uint32_t, std::endian::big>(maxCodeLengthInBits);
			Entry& entry = entries[entryIndex];
			
			if (!entry.bitsUsed)
			{
				throw std::runtime_error("TRV::ZLIB::DECOMPRESS encountered unpopulated entry in huffman table.");
			}
			T result = entry.symbol;
			consumer.discard_bits(entry.bitsUsed);

			return result;
		};

		uint32_t maxCodeLengthInBits;
		std::vector<Entry> entries;
	};

	void decompress(DeflateArgs& args);
}