#pragma once
#include <vector>
#include <thread>
#include "Common.h"
#include "WorkerPool.h"

namespace trv
{

	struct IHDR;
	struct PLTE;

	template <typename T>
	struct FilterArgs
	{
		std::vector<unsigned char>& input;
		IHDR& header;
		PLTE* palette;
		std::vector<T>& output;
	};

	void do_unfilter(std::vector<unsigned char>& input, size_t offset, size_t scanlines, size_t byteWidth, size_t bpp);

	template <std::integral InputType, std::integral OutputType>
	inline OutputType convertBitDepth(InputType val, OutputType inputBitDepth)
	{
		return static_cast<OutputType>((val * ((1ul << sizeof(OutputType)) - 1ul)) / ((1ul << sizeof(InputType)) - 1ul));
	}

	template <std::integral T>
	void unfilter(FilterArgs<T>& args)
	{
		IHDR& header = args.header;
		InterlaceMethod method { header.interlaceMethod };

		size_t channels = ((header.colorType & static_cast<uint8_t>(ColorType::Color)) + 1) +
			((header.colorType & static_cast<uint8_t>(ColorType::Alpha)) >> 2);

		assert(channels <= 4);

		bool usesPalette = header.colorType & static_cast<uint8_t>(ColorType::Palette);
		size_t bitsPerPixel = header.bitDepth * (usesPalette ? 1 : channels);
		channels = usesPalette ? 3 : channels;

		BitConsumer<std::endian::big> unfilteredConsumer(args.input);


		if (method == InterlaceMethod::None)
		{
			args.output.reserve(header.width * header.height * channels);
			size_t byteWidth = (header.width * bitsPerPixel + 7) / 8;

			if (byteWidth)
			{
				byteWidth += 1;
			}


			if constexpr (true)
			{
				// Multithreading support, tests show no speedup on large files, slowdown on small images
				assert(header.height != 0);
				WorkerPool workers(do_unfilter);
				int64_t scanline = 1;
				size_t chunkStart = 0;

				for (; scanline < header.height - 1; ++scanline)
				{
					FilterMethod currFilter = static_cast<FilterMethod>(args.input[scanline * byteWidth]);
					FilterMethod prevFilter = static_cast<FilterMethod>(args.input[(scanline - 1) * byteWidth]);

					bool isIndependent = currFilter == FilterMethod::None || currFilter == FilterMethod::Sub;
					if (isIndependent)
					{
						workers.AddTask(args.input, chunkStart * byteWidth, scanline - chunkStart, byteWidth, (bitsPerPixel + 7) / 8);
						chunkStart = scanline;
					}
				}

				// Last task will always escape out
				workers.AddTask(args.input, chunkStart * byteWidth, header.height - chunkStart, byteWidth, (bitsPerPixel + 7) / 8);

				while (workers.Busy())
				{
					std::this_thread::sleep_for(std::chrono::duration<double, std::milli>{20.0});
				}
			}
			else
			{
				do_unfilter(args.input, 0, header.height, byteWidth, (bitsPerPixel + 7) / 8);
			}


			for (size_t scanline = 0; scanline < header.height; ++scanline)
			{
				unfilteredConsumer.flush_byte();
				for (size_t component = 0; component < header.width * bitsPerPixel; component += header.bitDepth)
				{
					if (header.bitDepth <= 8)
					{
						uint8_t val = unfilteredConsumer.consume_bits<uint8_t, std::endian::big>(header.bitDepth);
						if (!usesPalette)
						{
							args.output.push_back(convertBitDepth<uint8_t, T>(val, header.bitDepth));
						}
						else
						{
							args.output.push_back(args.palette->data[val * 3]);
							args.output.push_back(args.palette->data[val * 3 + 1]);
							args.output.push_back(args.palette->data[val * 3 + 2]);
						}
					}
					else
					{
						uint16_t val = unfilteredConsumer.consume_bits<uint16_t, std::endian::big>(header.bitDepth);
						args.output.push_back(convertBitDepth<uint16_t, T>(val, header.bitDepth));
					}
				}
			}
		}
		else if (method == InterlaceMethod::Adam7)
		{
			static constexpr int rowStart[7] { 0, 0, 4, 0, 2, 0, 1 };
			static constexpr int colStart[7] { 0, 4, 0, 2, 0, 1, 0 };
			static constexpr int rowStride[7]{ 8, 8, 8, 4, 4, 2, 2 };
			static constexpr int colStride[7]{ 8, 8, 4, 4, 2, 2, 1 };

			size_t offset = 0;

			args.output.resize(header.width * header.height * channels);

			for (int pass = 0; pass < 7; ++pass)
			{
				size_t passWidth = (header.width  + colStride[pass] - 1 - colStart[pass]) / colStride[pass];
				size_t passHeight = (header.height + rowStride[pass] - 1 - rowStart[pass]) / rowStride[pass];
				size_t byteWidth = (passWidth * bitsPerPixel + 7) / 8;

				if (!byteWidth)
					continue;

				byteWidth += 1;
				do_unfilter(args.input, offset, passHeight, byteWidth, (bitsPerPixel + 7) / 8);

				for (size_t inRow = 0; inRow < passHeight; ++inRow)
				{
					unfilteredConsumer.flush_byte();
					for (size_t inCol = 0; inCol < passWidth; ++inCol)
					{
						size_t outRow = (inRow * rowStride[pass] + rowStart[pass]);
						size_t outCol = (inCol * colStride[pass] + colStart[pass]) * channels;

						if (!usesPalette)
						{
							for (size_t channel = 0; channel < channels; ++channel)
							{
								assert(args.output[outRow * header.width * channels + outCol + channel] == 0);
								if (header.bitDepth <= 8)
								{
									uint8_t val = unfilteredConsumer.consume_bits<uint8_t, std::endian::big>(header.bitDepth);
									args.output[outRow * header.width * channels + outCol + channel] = convertBitDepth<uint8_t, T>(val, header.bitDepth);
								}
								else
								{
									uint16_t val = unfilteredConsumer.consume_bits<uint16_t, std::endian::big>(header.bitDepth);
									args.output[outRow * header.width * channels + outCol + channel] = convertBitDepth<uint16_t, T>(val, header.bitDepth);
								}
							}
						}
						else
						{
							uint8_t val = unfilteredConsumer.consume_bits<uint8_t, std::endian::big>(header.bitDepth);
							args.output[outRow * header.width * channels + outCol] = convertBitDepth<uint8_t, T>(args.palette->data[val * 3], 8);
							args.output[outRow * header.width * channels + outCol + 1] = convertBitDepth<uint8_t, T>(args.palette->data[val * 3 + 1], 8);
							args.output[outRow * header.width * channels + outCol + 2] = convertBitDepth<uint8_t, T>(args.palette->data[val * 3 + 2], 8);
						}
					}
				}

				offset += byteWidth * passHeight;
			}
		}
		else
		{
			throw std::runtime_error("TRV::FILTER::UNFILTER - Encountered unexpected filter type.");
		}
	}
}