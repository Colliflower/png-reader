#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <type_traits>
#include <memory>
#include <assert.h>
#include "Zlib.h"
#include "Filter.h"
#include "CRC.h"

#include "Common.h"

namespace trv
{
	//Expected first 8 bytes of all PNG files
	static constexpr uint64_t header_signature = 0x89504e470d0a1a0a;
	static CRCTable<uint32_t> CRC32Table(CRCTable<uint32_t>::CRC_32);

	// Output type
	template <std::integral T>
	struct Image
	{
		Image(std::vector<T> data, uint32_t width, uint32_t height, uint32_t channels) :
			data(data),
			width(width),
			height(height),
			channels(channels)
		{};

		std::vector<T> data;
		uint32_t width, height, channels;
	};

	// Compile-time encoding of chunk types
	constexpr uint32_t encode_type(const char* str)
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

	// Enum of supported chunk types
	enum class ChunkType : size_t { IHDR, PLTE, IDAT, IEND, Count, Unknown };

	// Chunk data type, has requirements for generic construction
	template<typename T>
	concept IsChunk = requires (T x)
	{
		requires std::is_constructible_v<T, std::basic_ifstream<char>&, uint32_t>;
	};

	// Generic container for chunk in PNG with auxilliary data
	template <IsChunk T>
	struct Chunk
	{
		Chunk(std::basic_ifstream<char>& input, uint32_t size, uint32_t type) :
			size(size), 
			type(type),
			data(input, size),
			crc(extract_from_ifstream<uint32_t>(input))
		{
			uint32_t computed_crc = data.getCRC();
			if (computed_crc != crc)
			{
				std::stringstream msg;
				msg << "Computed CRC " << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << computed_crc
					<< " doesn't match read CRC " << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << crc;
				throw std::runtime_error(msg.str());
			}
		};

		void append(std::basic_ifstream<char>& input, uint32_t chunkSize)
		{
			data.append(input, chunkSize);
			uint32_t file_crc = extract_from_ifstream<uint32_t>(input);
			uint32_t computed_crc = data.getCRC();

			if (computed_crc != file_crc)
			{
				std::stringstream msg;
				msg << "Computed CRC " << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << computed_crc
					<< " doesn't match read CRC " << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << file_crc;
				throw std::runtime_error(msg.str());
			}
		}

		uint32_t size;
		uint32_t type;
		T data;
		uint32_t crc;
	};


	struct IHDR
	{
		constexpr static ChunkType type = ChunkType::IHDR;
		constexpr static char typeStr[] = { 'I','H','D','R' };

		IHDR() = default;

		IHDR(std::basic_ifstream<char>& input, uint32_t)
		{
			width = extract_from_ifstream<uint32_t>(input);
			height = extract_from_ifstream<uint32_t>(input);
			bitDepth = extract_from_ifstream<uint8_t>(input);
			colorType = extract_from_ifstream<uint8_t>(input);
			compressionMethod = extract_from_ifstream<uint8_t>(input);
			filterMethod = extract_from_ifstream<uint8_t>(input);
			interlaceMethod = extract_from_ifstream<uint8_t>(input);

			lastCRC = CRC32Table.Calculate<char, 4>(typeStr);
			uint32_t temp = big_endian<uint32_t>(width);
			lastCRC = CRC32Table.Calculate<uint32_t>(temp, lastCRC);
			temp = big_endian<uint32_t>(height);
			lastCRC = CRC32Table.Calculate<uint32_t>(temp, lastCRC);
			lastCRC = CRC32Table.Calculate<uint8_t>(bitDepth, lastCRC);
			lastCRC = CRC32Table.Calculate<uint8_t>(colorType, lastCRC);
			lastCRC = CRC32Table.Calculate<uint8_t>(compressionMethod, lastCRC);
			lastCRC = CRC32Table.Calculate<uint8_t>(filterMethod, lastCRC);
			lastCRC = CRC32Table.Calculate<uint8_t>(interlaceMethod, lastCRC);

		};

		uint32_t getCRC()
		{
			return lastCRC;
		}

		uint32_t lastCRC;
		uint32_t width;
		uint32_t height;
		uint8_t bitDepth;
		uint8_t colorType;
		uint8_t compressionMethod;
		uint8_t filterMethod;
		uint8_t interlaceMethod;
	};


	struct PLTE
	{
		constexpr static ChunkType type = ChunkType::PLTE;
		constexpr static char typeStr[] = { 'P','L','T','E' };

		PLTE(std::basic_ifstream<char>& input, uint32_t size) :
			data(size)
		{
			input.read(reinterpret_cast<char*>(data.data()), size);

			lastCRC = CRC32Table.Calculate<char, 4>(typeStr);
			lastCRC = CRC32Table.Calculate<uint8_t>(data.data(), size, lastCRC);
		};

		uint32_t getCRC()
		{
			return lastCRC;
		}

		uint32_t lastCRC;
		std::vector<uint8_t> data;
	};


	struct IDAT
	{
		constexpr static ChunkType type = ChunkType::IDAT;
		constexpr static char typeStr[] = { 'I','D','A','T' };

		IDAT(std::basic_ifstream<char>& input, uint32_t size) :
			data(size),
			lastCRC()
		{
			input.read(reinterpret_cast<char*>(data.data()), size);

			lastCRC = CRC32Table.Calculate<char, 4>(typeStr);
			lastCRC = CRC32Table.Calculate<uint8_t>(data.data(), size, lastCRC);
		};

		void append(std::basic_ifstream<char>& input, uint32_t size)
		{
			data.resize(data.size() + size);
			input.read(reinterpret_cast<char *>(data.data() + data.size() - size), size);

			lastCRC = CRC32Table.Calculate<char, 4>(typeStr);
			lastCRC = CRC32Table.Calculate<uint8_t>(data.data() + data.size() - size, size, lastCRC);
		}

		uint32_t getCRC()
		{
			return lastCRC;
		}

		uint32_t lastCRC;
		std::vector<uint8_t> data;
	};


	struct IEND
	{
		constexpr static ChunkType type = ChunkType::IEND;
		constexpr static char typeStr[] = { 'I','E','N','D' };
		IEND(std::basic_ifstream<char>&, uint32_t) {};

		constexpr uint32_t getCRC() const
		{
			return 0xAE426082;
		}
	};

	// Container for supported chunk types
	struct Chunks
	{
		Chunks() {};
		std::unique_ptr<Chunk<IHDR>> header;
		std::unique_ptr<Chunk<PLTE>> palette;
		std::unique_ptr<Chunk<IDAT>> image_data;
		std::unique_ptr<Chunk<IEND>> end;
	};

	// Read PNG file
	template <std::integral T>
	Image<T> load_image(const std::string& path)
	{
		std::ifstream infile(path, std::ios_base::binary | std::ios_base::in);
		if (infile.rdstate() & std::ios_base::failbit)
		{
			throw std::runtime_error("TRV::IMAGE::LOAD_IMAGE - Unable to open Image.");
		}
		// First thing's first

		uint64_t file_header = extract_from_ifstream<uint64_t>(infile);

		if (file_header != header_signature)
		{
			throw std::runtime_error("TRV::IMAGE::LOAD_IMAGE - Invalid PNG header.");
		}

		Chunks chunks;
		std::vector<ChunkType> sequence;

		while (infile.peek() != EOF)
		{
			uint32_t size = extract_from_ifstream<uint32_t>(infile);

			uint32_t type = extract_from_ifstream<uint32_t>(infile);

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
					uint32_t temp_type = big_endian<uint32_t>(type);
					char cType[5] = {0};
					memcpy(cType, &temp_type, 4);
					infile.seekg(size + sizeof(uint32_t), std::ios_base::cur);
 					std::cout << "TRV::IMAGE::LOAD_IMAGE - Skipping unhandled type " << cType << "\n";
					sequence.push_back(ChunkType::Unknown);
				}
			}
		}

		std::vector<uint8_t> decompressed;
		DeflateArgs decompressArgs { true, chunks.image_data->data.data, decompressed};
		
		decompress(decompressArgs);

		IHDR& header = chunks.header->data;
		std::vector<T> output;

		Chunk<PLTE>* paletteChunk = chunks.palette.get();

		PLTE* palette = nullptr;

		if (paletteChunk)
		{
			palette = &paletteChunk->data;
		}

		FilterArgs unfilterArgs = { decompressed, header, palette, output};
		unfilter<T>(unfilterArgs);

		size_t channels = (header.colorType & static_cast<uint8_t>(ColorType::Color)) + 1 +
                          ((header.colorType & static_cast<uint8_t>(ColorType::Alpha)) >> 2);
		bool usesPalette = header.colorType & static_cast<uint8_t>(ColorType::Palette);
		channels = usesPalette ? 3 : channels;

		assert(output.size() == header.width * header.height * channels);

		return Image<T>(output, chunks.header->data.width, chunks.header->data.height, static_cast<uint32_t>(channels));
	}
}
