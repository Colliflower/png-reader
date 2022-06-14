#include "Zlib.hpp"

namespace trv
{
void decompress(DeflateArgs& args)
{
	BitConsumer<std::endian::big> zlibConsumer(args.input);

	std::uint8_t CMF = zlibConsumer.consume_bits<uint8_t, std::endian::big>(8);

	if ((CMF & CMFilter) != CM)
	{
		throw std::runtime_error("TRV::ZLIB::DECOMPRESS CM must be 8");
	}

	std::uint8_t CINFO = (CMF & CINFOFilter) >> CINFOOffset;

	if (CINFO > 7)
	{
		throw std::runtime_error("TRV::ZLIB::DECOMPRESS CINFO cannot be larger than 7");
	}

	unsigned long window = 1L << (CINFO + 8);

	std::uint8_t FLG = zlibConsumer.consume_bits<uint8_t, std::endian::big>(8);

	std::uint16_t check = ((uint16_t)CMF * 256) + static_cast<uint16_t>(FLG);

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
		std::uint32_t FDICT = zlibConsumer.consume_bits<uint32_t, std::endian::big>(32);

		// TODO: Understand what to use this for.
	}

	std::vector<unsigned char>& output = args.output;

	BitConsumer<std::endian::little> deflateConsumer(zlibConsumer);

	bool is_final = false;
	while (!is_final)
	{
		is_final = deflateConsumer.consume_bits<uint8_t, std::endian::little>(1);
		enum BTYPES type =
		    static_cast<BTYPES>(deflateConsumer.consume_bits<uint8_t, std::endian::little>(2));

		if (type == BTYPES::None)
		{
			deflateConsumer.flush_byte();
			std::uint16_t len  = deflateConsumer.consume_bits<uint16_t, std::endian::little>(16);
			std::uint16_t nlen = deflateConsumer.consume_bits<uint16_t, std::endian::little>(16);

			if ((len ^ 0xFFFF) != nlen)
			{
				throw std::runtime_error(
				    "TRV::ZLIB::DECOMPRESS Unable to read properly, LEN and NLEN "
				    "don't line up.");
			}

			for (int i = 0; i < len; ++i)
			{
				output.push_back(deflateConsumer.consume_bits<uint8_t, std::endian::little>(8));
			}
		}
		else if (type == BTYPES::Err)
		{
			throw std::runtime_error(
			    "TRV::ZLIB::DECOMPRESS Encountered unexpected block type "
			    "3(Err).");
		}
		else
		{
			std::unique_ptr<Huffman<uint32_t>> LitLenHuffman, DistHuffman;
			if (type == BTYPES::DynamicHuff)
			{
				std::uint16_t HLIT =
				    deflateConsumer.consume_bits<uint16_t, std::endian::little>(5) + 257;
				std::uint16_t HDIST =
				    deflateConsumer.consume_bits<uint16_t, std::endian::little>(5) + 1;
				std::uint16_t HCLEN =
				    deflateConsumer.consume_bits<uint16_t, std::endian::little>(4) + 4;

				std::array<size_t, 19> HCLENSwizzle = { 16, 17, 18, 0, 8,  7, 9,  6, 10, 5,
					                                    11, 4,  12, 3, 13, 2, 14, 1, 15 };

				std::array<uint16_t, 19> HCLENTable {};

				for (uint32_t i = 0; i < HCLEN; ++i)
				{
					HCLENTable[HCLENSwizzle[i]] =
					    deflateConsumer.consume_bits<uint16_t, std::endian::little>(3);
				}

				Huffman<uint16_t> dictHuffman(8, 19, HCLENTable.data());
				std::uint16_t litLenCount = 0;
				std::uint16_t lenCount    = HLIT + HDIST;
				std::vector<uint32_t> litLenDistTable(lenCount);

				while (litLenCount < lenCount)
				{
					std::uint16_t repetitions = 1;
					std::uint16_t repeated    = 0;
					std::uint16_t encodedLen  = dictHuffman.decode(deflateConsumer);

					if (encodedLen <= 15)
					{
						repeated = encodedLen;
					}
					else if (encodedLen == 16)
					{
						if (!litLenCount)
						{
							throw std::runtime_error(
							    "TRV::ZLIB::DECOMPRESS Repeat code found on "
							    "first pass, therefore nothing can be repeated.");
						}
						repetitions =
						    deflateConsumer.consume_bits<uint16_t, std::endian::little>(2) + 3;
#pragma warning( \
    suppress : 6385)  // 16 cannot appear before <= 15 according to the deflate specificaition ^ I check above in case of corrupt data.
						repeated = static_cast<uint16_t>(litLenDistTable[litLenCount - 1]);
					}
					else if (encodedLen == 17)
					{
						repetitions =
						    deflateConsumer.consume_bits<uint16_t, std::endian::little>(3) + 3;
					}
					else if (encodedLen == 18)
					{
						repetitions =
						    deflateConsumer.consume_bits<uint16_t, std::endian::little>(7) + 11;
					}
					else
					{
						throw std::runtime_error(
						    "TRV::ZLIB::DECOMPRESS Unexpected encoded length.");
					}

					while (repetitions--)
					{
						litLenDistTable[litLenCount++] = repeated;
					}
				}

				LitLenHuffman =
				    std::make_unique<Huffman<uint32_t>>(16, HLIT, litLenDistTable.data());
				DistHuffman =
				    std::make_unique<Huffman<uint32_t>>(16, HDIST, litLenDistTable.data() + HLIT);
			}

			while (true)
			{
				std::uint32_t litLen;
				if (type == BTYPES::DynamicHuff)
				{
					litLen = LitLenHuffman->decode(deflateConsumer);
				}
				else
				{
					std::uint16_t code = deflateConsumer.peek_bits<uint16_t, std::endian::big>(9);
					std::uint16_t bits = 0;

					if (code >= FIXED_LIT_0_143_LOWER && code <= FIXED_LIT_0_143_UPPER)
					{
						litLen = (code >> (9 - FIXED_LIT_0_143_LENGTH)) - FIXED_LIT_0_143_ROOT +
						         FIXED_LIT_0_143_OFFSET;
						bits = FIXED_LIT_0_143_LENGTH;
					}
					else if (code >= FIXED_LIT_144_255_LOWER && code <= FIXED_LIT_144_255_UPPER)
					{
						litLen = (code >> (9 - FIXED_LIT_144_255_LENGTH)) - FIXED_LIT_144_255_ROOT +
						         FIXED_LIT_144_255_OFFSET;
						bits = FIXED_LIT_144_255_LENGTH;
					}
					else if (code >= FIXED_LIT_256_279_LOWER && code <= FIXED_LIT_256_279_UPPER)
					{
						litLen = (code >> (9 - FIXED_LIT_256_279_LENGTH)) - FIXED_LIT_256_279_ROOT +
						         FIXED_LIT_256_279_OFFSET;
						bits = FIXED_LIT_256_279_LENGTH;
					}
					else if (code >= FIXED_LIT_280_287_LOWER && code <= FIXED_LIT_280_287_UPPER)
					{
						litLen = (code >> (9 - FIXED_LIT_280_287_LENGTH)) - FIXED_LIT_280_287_ROOT +
						         FIXED_LIT_280_287_OFFSET;
						bits = FIXED_LIT_280_287_LENGTH;
					}
					else
					{
						throw std::runtime_error(
						    "TRV::ZLIB::DECOMPRES Invalid code encountered in "
						    "fixed huffman.");
					}

					deflateConsumer.discard_bits(bits);
				}

				if (litLen < 256)  // Literal
				{
					output.push_back(static_cast<uint8_t>(litLen));
				}
				else if (litLen >= 257)  // Length
				{
					assert(litLen <= 285);
					std::uint8_t lenIndex       = static_cast<uint8_t>(litLen - 257);
					std::uint16_t length        = lengthExtraTable[lenIndex * 2];
					std::size_t extraLengthBits = lengthExtraTable[lenIndex * 2 + 1];
					length += deflateConsumer.consume_bits<uint16_t, std::endian::little>(
					    extraLengthBits);

					std::uint32_t distIndex;

					if (type == BTYPES::DynamicHuff)
					{
						distIndex = DistHuffman->decode(deflateConsumer);
					}
					else
					{
						distIndex = deflateConsumer.consume_bits<uint32_t, std::endian::big>(5);
					}

					assert(distIndex < 30);
					std::uint16_t distance        = distanceExtraTable[distIndex * 2];
					std::size_t extraDistanceBits = distanceExtraTable[distIndex * 2 + 1];
					distance += deflateConsumer.consume_bits<uint16_t, std::endian::little>(
					    extraDistanceBits);

					assert(output.size() > distance);
					assert(distance < window);
					std::size_t offset = output.size() - distance;
					//output.reserve(output.size() + length);
					for (size_t from = offset; from < offset + length; ++from)
					{
						output.emplace_back(output[from]);
					}
				}
				else  // End of block
				{
					break;
				}
			}
		}
	}
}
}
