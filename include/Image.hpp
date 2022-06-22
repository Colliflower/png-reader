#pragma once

#include <assert.h>

#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "Chunk.hpp"
#include "Common.hpp"
#include "Filter.hpp"
#include "Zlib.hpp"
#include "utility/export.hpp"

namespace trv
{
//Expected first 8 bytes of all PNG files
static constexpr std::uint64_t header_signature = 0x89504e470d0a1a0a;

// Output type
template <std::integral T>
struct Image
{
	Image(std::vector<T> data, std::uint32_t width, std::uint32_t height, std::uint32_t channels) :
	    data(std::move(data)), width(width), height(height), channels(channels) {};

	std::vector<T> data;
	std::uint32_t width, height, channels;
};

// Compile-time encoding of chunk types
[[nodiscard]] constexpr std::uint32_t encode_type(const char* str)
{
	if constexpr (std::endian::native == std::endian::big)
	{
		return str[0] | str[1] << 8 | str[2] << 16 | str[3] << 24;
	}
	else
	{
		return str[3] | str[2] << 8 | str[1] << 16 | str[0] << 24;
	}
}

// Read PNG file
template <std::integral T>
[[nodiscard]] DLL_PUBLIC Image<T> load_image(const std::string& path)
{
	std::ifstream infile(path, std::ios_base::binary | std::ios_base::in);
	if (infile.rdstate() & std::ios_base::failbit)
	{
		throw std::runtime_error("TRV::IMAGE::LOAD_IMAGE - Unable to open Image.");
	}
	// First thing's first

	std::uint64_t file_header = extract_from_ifstream<uint64_t>(infile);

	if (file_header != header_signature)
	{
		throw std::runtime_error("TRV::IMAGE::LOAD_IMAGE - Invalid PNG header.");
	}

	Chunks chunks;
	std::vector<ChunkType> sequence;

	while (infile.peek() != EOF)
	{
		std::uint32_t size = extract_from_ifstream<uint32_t>(infile);

		std::uint32_t type = extract_from_ifstream<uint32_t>(infile);

		switch (type)
		{
			case encode_type("IHDR"):
				chunks.header = std::make_unique<Chunk<IHDR>>(infile, size, type);
				sequence.push_back(ChunkType::IHDR);
				break;
			case encode_type("PLTE"):
				chunks.palette = std::make_unique<Chunk<PLTE>>(infile, size, type);
				sequence.push_back(ChunkType::PLTE);
				break;
			case encode_type("IDAT"):
				if (chunks.image_data == nullptr)
				{
					chunks.image_data = std::make_unique<Chunk<IDAT>>(infile, size, type);
				}
				else
				{
					chunks.image_data->append(infile, size);
				}
				sequence.push_back(ChunkType::IDAT);
				break;
			case encode_type("IEND"):
				chunks.end = std::make_unique<Chunk<IEND>>(infile, size, type);
				sequence.push_back(ChunkType::IEND);
				break;
			default:
				{
					std::uint32_t temp_type = big_endian<uint32_t>(type);
					char cType[5]           = { 0 };
					memcpy(cType, &temp_type, 4);
					infile.seekg(size + sizeof(uint32_t), std::ios_base::cur);
					std::cout << "TRV::IMAGE::LOAD_IMAGE - Skipping unhandled type " << cType
					          << "\n";
					sequence.push_back(ChunkType::Unknown);
				}
		}
	}

	IHDR& header         = chunks.header->data;
	std::size_t channels = (header.colorType & static_cast<uint8_t>(ColorType::Color)) + 1 +
	                       ((header.colorType & static_cast<uint8_t>(ColorType::Alpha)) >> 2);
	bool usesPalette = header.colorType & static_cast<uint8_t>(ColorType::Palette);
	channels         = usesPalette ? 3 : channels;

	std::vector<unsigned char> decompressed;
	std::size_t bitsPerPixel = header.bitDepth * (usesPalette ? 1 : channels);
	std::size_t byteWidth    = (header.width * bitsPerPixel + 7) / 8;

	if (byteWidth)
	{
		byteWidth += 1;
	}

	decompressed.reserve(header.height * byteWidth);
	DeflateArgs decompressArgs { true, chunks.image_data->data.data, decompressed };

	decompress(decompressArgs);

	std::vector<T> output;

	Chunk<PLTE>* paletteChunk = chunks.palette.get();

	PLTE* palette = nullptr;

	if (paletteChunk)
	{
		palette = &paletteChunk->data;
	}

	FilterArgs unfilterArgs { decompressed, &header, palette, output };
	unfilter<T>(unfilterArgs);

	assert(output.size() == header.width * header.height * channels);

	return Image<T>(output, chunks.header->data.width, chunks.header->data.height,
	                static_cast<uint32_t>(channels));
}
}
