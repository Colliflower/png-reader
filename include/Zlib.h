#pragma once

#include <stdint.h>
#include "Common.h"
#include <algorithm>
#include <iterator>

namespace trv
{
	static const unsigned char CMFilter = 0x0F;
	static const unsigned char CM = 0x08;
	static const unsigned char CINFOFilter = 0xF0;
	static const unsigned char CINFOOffset = 4;
	static const unsigned char FDICTFilter = 0x20;
	static const unsigned char FLEVELFilter = 0xC0;
	static const unsigned char BFINAL = 0x01;
	static const unsigned char BTYPE = 0x06;

	enum class BTYPES : unsigned char { None = 0x00, FixedHuff = 0x01, DynamicHuff=0x02, Err=0x03 };

	struct DeflateArgs
	{
		bool png;
		std::vector<unsigned char >& output;
	};

	static unsigned char bitsConsumed = 0;


	//template <std::integral T>
	//T peek_bits_lsb(char** input, T bits)
	//{
	//	T val = 0;

	//	T bytesConsumed = 0;
	//	unsigned char prevBitsConsumed = bitsConsumed;

	//	while (bits)
	//	{
	//		T bitsAvailable = 8 - bitsConsumed;

	//		T amount = bits > bitsAvailable ? bitsAvailable : bits;
	//		// Shift current val by 8

	//		T current = (((**input) >> bitsConsumed) & ((1 << amount) - 1));
	//		bitsConsumed += amount;
	//		bits -= amount;
	//		val |= current << bits;

	//		if (bitsConsumed == 8)
	//		{
	//			(*input)++;
	//			bitsConsumed = 0;
	//			bytesConsumed++;
	//		}
	//	}

	//	*input -= bytesConsumed;
	//	bitsConsumed = prevBitsConsumed;

	//	return little_endian<T>(val);
	//}

	template <std::integral T>
	T peek_bits_msb(char** input, T bits)
	{
		T val = 0;

		T bytesConsumed = 0;
		unsigned char prevBitsConsumed = bitsConsumed;
		unsigned char shift = sizeof(T) * 8;
		unsigned char final_shift = shift - bits;

		while (bits)
		{
			T bitsAvailable = 8 - bitsConsumed;

			T amount = bits > bitsAvailable ? bitsAvailable : bits;
			unsigned char masked = static_cast<unsigned char>(**input) & ~((1 << bitsConsumed) - 1);
			T current = static_cast<T>((masked * 0x0202020202ULL & 0x010884422010ULL) % 1023); // reverse bits
			shift = shift - 8 + bitsConsumed;
			current = current << shift;
			bits -= amount;
			bitsConsumed += amount;
			val |= current;

			if (bitsConsumed == 8)
			{
				(*input)++;
				bitsConsumed = 0;
				bytesConsumed++;
			}
		}

		// TODO(trevor): This is the problem. rethink how thecodes work
		// you shift in 7 bits but later find out you only need 3. If you shift right then it doesn't work.

		val = val >> final_shift;

		*input -= bytesConsumed;
		bitsConsumed = prevBitsConsumed;

		return little_endian<T>(val);
	}
	
	template <std::integral T>
	void discard_bits(char** input, T bits)
	{
		while (bits)
		{
			T bitsAvailable = 8 - bitsConsumed;
			T amount = bits > bitsAvailable ? bitsAvailable : bits;
			bitsConsumed += amount;
			bits -= amount;

			if (bitsConsumed == 8)
			{
				(*input)++;
				bitsConsumed = 0;
			}
		}
	}

	template <std::integral T>
	T consume_bits_lsb(char** input, T bits)
	{
		T val = 0;

		while (bits)
		{
			T bitsAvailable = 8 - bitsConsumed;

			T amount = bits > bitsAvailable ? bitsAvailable : bits;
			// Shift current val by 8

			T current = (((**input) >> bitsConsumed) & ((1 << amount) - 1));
			bitsConsumed += amount;
			bits -= amount;
			val |= current << bits;

			if (bitsConsumed == 8)
			{
				(*input)++;
				bitsConsumed = 0;
			}
		}

		return little_endian<T>(val);
	}

	template <std::integral T>
	T consume_bits_msb(char** input, T bits)
	{
		T val = 0;

		unsigned char prevBitsConsumed = bitsConsumed;
		unsigned char shift = sizeof(T) * 8;
		unsigned char final_shift = shift - bits;

		while (bits)
		{
			T bitsAvailable = 8 - bitsConsumed;

			T amount = bits > bitsAvailable ? bitsAvailable : bits;
			unsigned char masked = static_cast<unsigned char>(**input) & ~((1 << bitsConsumed) - 1);
			T current = static_cast<T>((masked * 0x0202020202ULL & 0x010884422010ULL) % 1023); // reverse bits
			shift = shift - 8 + bitsConsumed;
			current = current << shift;
			bits -= amount;
			bitsConsumed += amount;
			val |= current;

			if (bitsConsumed == 8)
			{
				(*input)++;
				bitsConsumed = 0;
			}
		}

		val = val >> final_shift;

		return little_endian<T>(val);
	}

	inline void flush_byte(char** input)
	{
		(*input)++;
		bitsConsumed = 0;
	}

	static const uint16_t lengthExtraTable[29 * 2] =
	{
	//Initial ExtraBits
		  3,      0,
		  4,      0,
		  5,      0,
		  6,      0,
		  7,      0,
		  8,      0,
		  9,      0,
		 10,      0,
		 11,      1,
		 13,      1,
		 15,      1,
		 17,      1,
		 19,      2,
		 23,      2,
		 27,      2,
		 31,      2,
		 35,      3,
		 43,      3,
		 51,      3,
		 59,      3,
		 67,      4,
		 83,      4,
		 99,      4,
		115,      4,
		131,      5,
		163,      5,
		195,      5,
		227,      5,
		258,      0
	};

	static const uint16_t distanceExtraTable[30*2] =
	{
    //Initial ExtraBits
			1,    0,
			2,    0,
			3,    0,
			4,    0,
			5,    1,
			7,    1,
			9,    2,
		   13,    2,
		   17,    3,
		   25,    3,
		   33,    4,
		   49,    4,
		   65,    5,
		   97,    5,
		  129,    6,
		  193,    6,
		  257,    7,
		  385,    7,
		  513,    8,
		  769,    8,
		 1025,    9,
		 1537,    9,
		 2049,   10,
		 3073,   10,
		 4097,   11,
		 6145,   11,
		 8193,   12,
		12289,   12,
		16385,   13,
		24577,   13
	};

	template <std::integral T>
	struct Huffman
	{
		struct Entry
		{
			T symbol;
			uint16_t bitsUsed;
		};

		Huffman(unsigned char maxCodeLengthInBits, uint32_t symbolCount, T* symbolCodeLength, T symbolAddend = 0) :
			maxCodeLengthInBits(maxCodeLengthInBits),
			entries(static_cast<uint64_t>(1) << maxCodeLengthInBits)
		{
 			std::vector<T> codeLengthHistogram(maxCodeLengthInBits);

			for (uint32_t symbolIndex = 0; symbolIndex < symbolCount; ++symbolIndex)
			{
				assert(symbolCodeLength[symbolIndex] < maxCodeLengthInBits);
				++codeLengthHistogram[symbolCodeLength[symbolIndex]];
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

				uint32_t prependBits = maxCodeLengthInBits - codeLengthInBits;
				for (uint32_t prepend = 0; prepend < (1u << prependBits); ++prepend)
				{
					uint32_t entryIndex = (prepend << codeLengthInBits)| code;
					Entry& entry = entries[entryIndex];
					entry.symbol = symbolIndex + symbolAddend;

					assert(codeLengthInBits < sizeof(T) * 8);
					entry.bitsUsed = codeLengthInBits;
				}
			}
		};

		T decode(char** input)
		{	  
			uint32_t entryIndex = peek_bits_msb<T>(input, maxCodeLengthInBits);
			Entry& entry = entries[entryIndex];
			
			if (!entry.bitsUsed)
			{
				throw std::runtime_error("TRV::ZLIB::DECOMPRESS encountered unpopulated entry in huffman table.");
			}
			T result = entry.symbol;
			discard_bits<T>(input, entry.bitsUsed);

			return result;
		};

		uint32_t maxCodeLengthInBits;
		std::vector<Entry> entries;
	};

	inline void decompress(char* input, DeflateArgs& args)
	{
		unsigned char CMF = extract_from_msb_bytes<unsigned char>(&input);

		if ((CMF & CMFilter) != CM)
		{
			throw std::runtime_error("TRV::ZLIB::DECOMPRESS CM must be 8");
		}

		unsigned char CINFO = (CMF & CINFOFilter) >> CINFOOffset;

		if (CINFO > 7)
		{
			throw std::runtime_error("TRV::ZLIB::DECOMPRESS CINFO cannot be larger than 7");
		}

		unsigned long width = 1L << (CINFO + 8);

		unsigned char FLG = extract_from_msb_bytes<unsigned char>(&input);

		uint16_t check = ((uint16_t)CMF * 256) + static_cast<uint16_t>(FLG);

		if (check % 31 != 0)
		{
			throw std::runtime_error("TRV::ZLIB::DECOMPRESS FLGCHECK failed");
		}

		if (FLG & FDICTFilter && args.png)
		{
			throw std::runtime_error("TRV::ZLIB::DECOMPRESS FDICT cannot be set in PNG files.");
		}
		else if (FLG & FDICTFilter)
		{
			unsigned long FDICT = extract_from_msb_bytes<unsigned long>(&input);
			
			// TODO: Understand what to use this for.
		}

		std::vector<unsigned char>& output = args.output;

		bool is_final = false;
		while (!is_final)
		{
			is_final = consume_bits_lsb<unsigned char>(&input, 1);
			enum BTYPES type = static_cast<BTYPES>(consume_bits_lsb<unsigned char>(&input, 2));

			if (type == BTYPES::None)
			{
				flush_byte(&input);
				uint16_t len = extract_from_lsb_bytes<uint16_t>(&input);
				uint16_t nlen = extract_from_lsb_bytes<uint16_t>(&input);

 				if ((len ^ 0xFFFF) != nlen)
				{
					throw std::runtime_error("TRV::ZLIB::DECOMPRESS Unable to read properly, LEN and NLEN don't line up.");
				}
				output.reserve(output.size() + len);
				output.insert(output.end(), input, input + len);
				input += len;
			}
			else if (type == BTYPES::Err)
			{
				throw std::runtime_error("TRV::ZLIB::DECOMPRESS Encountered unexpected block type 3(Err).");
			}
			else
			{
				uint16_t HLIT = consume_bits_lsb<uint32_t>(&input, 5) + 257;
				uint16_t HDIST = consume_bits_lsb<uint32_t>(&input, 5) + 1;
				uint16_t HCLEN = consume_bits_lsb<uint32_t>(&input, 4) + 4;

				size_t HCLENSwizzle[] =
				{
					16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
				};

				uint16_t HCLENTable[19] = {};

				for (uint32_t i = 0; i < HCLEN; ++i)
				{
					HCLENTable[HCLENSwizzle[i]] = consume_bits_lsb<uint16_t>(&input, 3);
				}

				Huffman<uint16_t> dictHuffman(7, 19, HCLENTable);
				uint32_t litLenDistTable[512] = {};
				uint16_t litLenCount = 0;
				uint16_t lenCount = HLIT + HDIST;

				while (litLenCount < lenCount)
				{
					uint16_t repetitions = 1;
					uint16_t repeated = 0;
 					uint16_t encodedLen = dictHuffman.decode(&input);

					if (encodedLen <= 15)
					{
						litLenDistTable[litLenCount++] = encodedLen;
					}
					else if (encodedLen == 16)
					{
						if (!litLenCount)
						{
							throw std::runtime_error("TRV::ZLIB::DECOMPRESS Repeat code found on first pass, therefore nothing can be repeated.");
						}
						repetitions = consume_bits_lsb<uint32_t>(&input, 2) + 3;
#pragma warning ( suppress: 6385 ) // 16 cannot appear before <= 15 according to the deflate specificaition ^ I check there in case of corrupt data.
						repeated = litLenDistTable[litLenCount - 1];

					}
					else if (encodedLen == 17)
					{
						repetitions = consume_bits_lsb<uint32_t>(&input, 3) + 3;
					}
					else if (encodedLen == 18)
					{
						repetitions = consume_bits_lsb<uint32_t>(&input, 7) + 11;
					}
					else
					{
						throw std::runtime_error("TRV::ZLIB::DECOMPRESS Unexpected encoded length.");
					}

					while (repetitions--)
					{
						litLenDistTable[litLenCount++] = repeated;
					}
				}

				Huffman<uint32_t> LitLenHuffman(15, HLIT, litLenDistTable);
				Huffman<uint32_t> DistHuffman(15, HDIST, litLenDistTable + HLIT);

				uint32_t litLen = LitLenHuffman.decode(&input);

				if (litLen < 256) // Literal
				{
					output.push_back(static_cast<unsigned char>(litLen));
				}
				else if (litLen >= 257) // Length
				{
					assert(litLen <= 285);
					unsigned char lenIndex = litLen - 257;
					uint16_t length = lengthExtraTable[lenIndex * 2];
					length += consume_bits_lsb<uint16_t>(&input, lengthExtraTable[lenIndex * 2 + 1]);

					uint32_t distIndex = DistHuffman.decode(&input);
					assert(distIndex < 30);
					uint16_t distance = distanceExtraTable[distIndex * 2];
					distance += consume_bits_lsb<uint16_t>(&input, distanceExtraTable[distIndex * 2 + 1]);

					assert(output.size() > distance);
					size_t offset = output.size() - distance;
					output.reserve(output.size() + length);
					std::copy_n(output.begin() + offset, length, std::back_inserter(output));
				}
				else // End of block
				{
					break;
				}
			}

			return;
		}
	};
}