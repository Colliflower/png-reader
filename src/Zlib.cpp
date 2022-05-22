#include "Zlib.h"

namespace trv
{
	void decompress(DeflateArgs& args)
	{
		BitConsumer<std::endian::big> zlibConsumer(args.input);

		uint8_t CMF = zlibConsumer.consume_bits<uint8_t, std::endian::big>(8);

		if ((CMF & CMFilter) != CM)
		{
			throw std::runtime_error("TRV::ZLIB::DECOMPRESS CM must be 8");
		}

		uint8_t CINFO = (CMF & CINFOFilter) >> CINFOOffset;

		if (CINFO > 7)
		{
			throw std::runtime_error("TRV::ZLIB::DECOMPRESS CINFO cannot be larger than 7");
		}

		unsigned long window = 1L << (CINFO + 8);

		uint8_t FLG = zlibConsumer.consume_bits<uint8_t, std::endian::big>(8);

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
			uint32_t FDICT = zlibConsumer.consume_bits<uint32_t, std::endian::big>(32);

			// TODO: Understand what to use this for.
		}

		std::vector<uint8_t>& output = args.output;

		BitConsumer<std::endian::little> deflateConsumer(zlibConsumer);

		bool is_final = false;
		while (!is_final)
		{
			is_final = deflateConsumer.consume_bits<uint8_t, std::endian::little>(1);
			enum BTYPES type = static_cast<BTYPES>(deflateConsumer.consume_bits<uint8_t, std::endian::little>(2));

			if (type == BTYPES::None)
			{
				deflateConsumer.flush_byte();
				uint16_t len = deflateConsumer.consume_bits<uint16_t, std::endian::little>(16);
				uint16_t nlen = deflateConsumer.consume_bits<uint16_t, std::endian::little>(16);

				if ((len ^ 0xFFFF) != nlen)
				{
					throw std::runtime_error("TRV::ZLIB::DECOMPRESS Unable to read properly, LEN and NLEN don't line up.");
				}

				// TODO: Add a method to BitConsumer to copy a length of data to an output vector;
				//output.reserve(output.size() + len);
				//output.insert(output.end(), input, input + len);
				//input += len;
			}
			else if (type == BTYPES::Err)
			{
				throw std::runtime_error("TRV::ZLIB::DECOMPRESS Encountered unexpected block type 3(Err).");
			}
			else
			{
				uint16_t HLIT = deflateConsumer.consume_bits<uint16_t, std::endian::little>(5) + 257;
				uint16_t HDIST = deflateConsumer.consume_bits<uint16_t, std::endian::little>(5) + 1;
				uint16_t HCLEN = deflateConsumer.consume_bits<uint16_t, std::endian::little>(4) + 4;

				std::array<size_t, 19> HCLENSwizzle =
				{
					16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
				};

				std::array<uint16_t,19> HCLENTable {};

				for (uint32_t i = 0; i < HCLEN; ++i)
				{
					HCLENTable[HCLENSwizzle[i]] = deflateConsumer.consume_bits<uint16_t, std::endian::little>(3);
				}

				Huffman<uint16_t> dictHuffman(7, 19, HCLENTable.data());
				uint16_t litLenCount = 0;
				uint16_t lenCount = HLIT + HDIST;
				std::vector<uint32_t> litLenDistTable(lenCount);

				while (litLenCount < lenCount)
				{
					uint16_t repetitions = 1;
					uint16_t repeated = 0;
					uint16_t encodedLen = dictHuffman.decode(deflateConsumer);

					if (encodedLen <= 15)
					{
						repeated = encodedLen;
					}
					else if (encodedLen == 16)
					{
						if (!litLenCount)
						{
							throw std::runtime_error("TRV::ZLIB::DECOMPRESS Repeat code found on first pass, therefore nothing can be repeated.");
						}
						repetitions = deflateConsumer.consume_bits<uint16_t, std::endian::little>(2) + 3;
#pragma warning ( suppress: 6385 ) // 16 cannot appear before <= 15 according to the deflate specificaition ^ I check there in case of corrupt data.
						repeated = static_cast<uint16_t>(litLenDistTable[litLenCount - 1]);

					}
					else if (encodedLen == 17)
					{
						repetitions = deflateConsumer.consume_bits<uint16_t, std::endian::little>(3) + 3;
					}
					else if (encodedLen == 18)
					{
						repetitions = deflateConsumer.consume_bits<uint16_t, std::endian::little>(7) + 11;
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

				Huffman<uint32_t> LitLenHuffman(15, HLIT, litLenDistTable.data());
				Huffman<uint32_t> DistHuffman(15, HDIST, litLenDistTable.data() + HLIT);

				while (true)
				{
					uint32_t litLen = LitLenHuffman.decode(deflateConsumer);

					if (litLen < 256) // Literal
					{
						output.push_back(static_cast<uint8_t>(litLen));
					}
					else if (litLen >= 257) // Length
					{
						assert(litLen <= 285);
						uint8_t lenIndex = static_cast<uint8_t>(litLen - 257);
						uint16_t length = lengthExtraTable[lenIndex * 2];
						size_t extraLengthBits = lengthExtraTable[lenIndex * 2 + 1];
						length += deflateConsumer.consume_bits<uint16_t, std::endian::little>(extraLengthBits);

						uint32_t distIndex = DistHuffman.decode(deflateConsumer);
						assert(distIndex < 30);
						uint16_t distance = distanceExtraTable[distIndex * 2];
						size_t extraDistanceBits = distanceExtraTable[distIndex * 2 + 1];
						distance += deflateConsumer.consume_bits<uint16_t, std::endian::little>(extraDistanceBits);

						assert(output.size() > distance);
						assert(distance < window);
						size_t offset = output.size() - distance;
						output.reserve(output.size() + length);
						for (int from = offset; from < offset + length; ++from)
						{
							output.push_back(output[from]);
						}
					}
					else // End of block
					{
						break;
					}
				}
			}

			return;
		}
	}
}
