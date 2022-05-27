#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <type_traits>
#include <memory>
#include <assert.h>
#include "Zlib.h"

#define CRCPP_USE_CPP11
#include "ext/CRC.h"

#include "Common.h"

namespace trv
{
	//Expected first 8 bytes of all PNG files
	static constexpr uint64_t header = 0x89504e470d0a1a0a;
	static const auto& crc_table = CRC::CRC_32().MakeTable();
	enum class InterlaceMethod : uint8_t { None, Adam7 };
	enum class FilterMethod : uint8_t {None, Sub, Up, Average, Paeth };
	enum class ColorType : uint8_t { Palette = 0b001, Color = 0b010, Alpha = 0b100 };

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

			lastCRC = CRC::Calculate(typeStr, sizeof(typeStr), crc_table);
			uint32_t temp = big_endian<uint32_t>(width);
			lastCRC = CRC::Calculate(reinterpret_cast<char*>(&temp), sizeof(uint32_t), crc_table, lastCRC);
			temp = big_endian<uint32_t>(height);
			lastCRC = CRC::Calculate(reinterpret_cast<char*>(&temp), sizeof(uint32_t), crc_table, lastCRC);
			lastCRC = CRC::Calculate(&bitDepth, sizeof(char)*5, crc_table, lastCRC);

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
			palette(size)
		{
			input.read(reinterpret_cast<char*>(palette.data()), size);

			lastCRC = CRC::Calculate(typeStr, sizeof(typeStr), crc_table);
			lastCRC = CRC::Calculate(palette.data(), size, crc_table, lastCRC);
		};

		uint32_t getCRC()
		{
			return lastCRC;
		}

		uint32_t lastCRC;
		std::vector<uint8_t> palette;
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

			lastCRC = CRC::Calculate(typeStr, sizeof(typeStr), crc_table);
			lastCRC = CRC::Calculate(data.data(), size, crc_table, lastCRC);
		};

		void append(std::basic_ifstream<char>& input, uint32_t size)
		{
			data.resize(data.size() + size);
			input.read(reinterpret_cast<char *>(data.data() + data.size() - size), size);

			lastCRC = CRC::Calculate(typeStr, sizeof(typeStr), crc_table);
			lastCRC = CRC::Calculate(data.data() + data.size() - size, size, crc_table, lastCRC);
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

	static uint8_t paethPredictor(uint8_t left, uint8_t top, uint8_t topleft)
	{
		int32_t p = static_cast<uint16_t>(left + top - topleft);
		int32_t pleft = p > left ? p - left : left - p;
		int32_t ptop = p > top ? p - top : top - p;
		int32_t ptopleft = p > topleft ? p - topleft : topleft - p;


		if (pleft <= ptop && pleft <= ptopleft)
		{
			return static_cast<uint8_t>(pleft);
		}
		else if (ptop <= ptopleft)
		{
			return static_cast<uint8_t>(ptop);
		}
		else
		{
			return static_cast<uint8_t>(ptopleft);
		}
	};

	// Container for supported chunk types
	struct Chunks
	{
		Chunks() {};
		std::unique_ptr<Chunk<IHDR>> header;
		std::unique_ptr<Chunk<PLTE>> pallette;
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

		if (file_header != header)
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
					chunks.pallette = std::make_unique<Chunk<PLTE>>(infile, size, type);
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
		DeflateArgs args = { true, chunks.image_data->data.data, decompressed};
		
		decompress(args);

		IHDR& header = chunks.header->data;

		size_t channels = ((header.colorType & static_cast<uint8_t>(ColorType::Color)) ? 3 : 1) + 
			              ((header.colorType & static_cast<uint8_t>(ColorType::Alpha)) ? 1 : 0);

		size_t bpp = header.bitDepth * (( header.colorType & static_cast<uint8_t>(ColorType::Palette)) ? 1 : channels);
		channels = (header.colorType & static_cast<uint8_t>(ColorType::Palette)) ? 3 : channels;
		bpp = (bpp + 7) / 8;

		size_t byteWidth = bpp * header.width + 1;

		assert(decompressed.size() == (byteWidth * header.height));

		std::vector<T> unfiltered;
		unfiltered.reserve(header.width * header.height * channels);

		switch (static_cast<InterlaceMethod>(header.interlaceMethod))
		{
		case InterlaceMethod::None:
		{
			for (size_t scanline = 0; scanline < header.height; ++scanline)
			{
				assert(decompressed[scanline * byteWidth] <= 4);
				FilterMethod filterType = static_cast<FilterMethod>(decompressed[scanline * byteWidth]);
				
				if (filterType == FilterMethod::None ||
					(filterType == FilterMethod::Up && scanline == 0))
				{
					std::copy(decompressed.begin() + (scanline * byteWidth + 1), decompressed.begin() + (scanline * byteWidth + byteWidth), std::back_inserter(unfiltered));
					continue;
				}

				for (size_t byte = 1; byte < byteWidth; ++byte)
				{
					switch (static_cast<FilterMethod>(filterType))
					{
					case FilterMethod::Sub:
						if (byte <= bpp)
						{
							unfiltered.push_back(decompressed[scanline * byteWidth + byte]);
						}
						else
						{
							unfiltered.push_back(decompressed[scanline * byteWidth + byte] + unfiltered[unfiltered.size() - bpp]);
						}
						break;
					case FilterMethod::Up:
						unfiltered.push_back(decompressed[scanline * byteWidth + byte] + decompressed[(scanline - 1) * byteWidth + byte]);
						break;
					case FilterMethod::Average:
					case FilterMethod::Paeth:
					{
						uint8_t top = 0;
						uint8_t left = 0;
						uint8_t topleft = 0;

						if (scanline != 0 && byte > 1)
							topleft = decompressed[(scanline - 1) * byteWidth + byte - bpp];

						if (scanline != 0)
							top = decompressed[(scanline - 1) * byteWidth + byte];

						if (byte > 1)
							left = decompressed[scanline * byteWidth + byte - bpp];

						if (left == 0 && left == top && top == topleft)
						{
							unfiltered.push_back(decompressed[scanline * byteWidth + byte]);
							break;
						}

						if (filterType == FilterMethod::Average)
							unfiltered.push_back(decompressed[scanline * byteWidth + byte] + static_cast<uint8_t>(static_cast<uint16_t>(top + left) / 2));
						else
							unfiltered.push_back(decompressed[scanline * byteWidth + byte] + paethPredictor(left, top, topleft));
						break;
					}
					default:
						throw std::runtime_error("TRV::IMAGE::LOAD_IMAGE Encountered unexpected filter type.");
						break;
					}
				}
				assert(unfiltered.size() == header.width * (scanline + 1)* channels);
			}
			break;
		}
		case InterlaceMethod::Adam7:
			break;
		default:
			throw std::runtime_error("Unexpected interlace method.");
			break;
		}

		assert(unfiltered.size() == header.width * header.height * channels);

		return Image<T>(unfiltered, chunks.header->data.width, chunks.header->data.height, channels);
	}
}
