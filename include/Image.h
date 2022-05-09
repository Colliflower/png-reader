#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <bit>
#include <type_traits>
#include <memory>
#include <assert.h>

#define CRCPP_USE_CPP11
#include "ext/CRC.h"

namespace trv
{
	//Expected first 8 bytes of all PNG files
	static const uint64_t header = 0x89504e470d0a1a0a;
	static const auto& crc_table = CRC::CRC_32().MakeTable();

	// Output type
	template <typename T>
	struct Image
	{
		std::vector<T> data;
		uint32_t width, height, channels;
	};

	// Allows reading of integral types from big-endian binary stream
	template <std::integral T>
	T extract_from_ifstream(std::basic_ifstream<char>& input) {
		T val;
		input.read(reinterpret_cast<char*>(&val), sizeof(val));

		if (std::endian::native == std::endian::little)
		{
			val = std::byteswap<T>(val);
		}

		return val;
	}

	template <std::integral T>
	T big_endian(T val)
	{
		if (std::endian::native == std::endian::little)
		{
			val = std::byteswap<T>(val);
		}

		return val;
	}
	// Compile-time encoding of chunk types
	constexpr uint32_t encode_type(const char* str)
	{
		if (std::endian::native == std::endian::big)
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

		void append(std::basic_ifstream<char>& input, uint32_t size)
		{
			data.append(input, size);
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

		uint32_t getSize()
		{
			return 3 * sizeof(uint32_t) + size;
		};

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

		IHDR(std::basic_ifstream<char>& input, uint32_t size)
		{
			width = extract_from_ifstream<uint32_t>(input);
			height = extract_from_ifstream<uint32_t>(input);
			bitDepth = extract_from_ifstream<char>(input);
			colorType = extract_from_ifstream<char>(input);
			compressionMethod = extract_from_ifstream<char>(input);
			filterMethod = extract_from_ifstream<char>(input);
			interlaceMethod = extract_from_ifstream<char>(input);

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
		char bitDepth;
		char colorType;
		char compressionMethod;
		char filterMethod;
		char interlaceMethod;
	};


	struct PLTE
	{
		constexpr static ChunkType type = ChunkType::PLTE;
		constexpr static char typeStr[] = { 'P','L','T','E' };

		PLTE(std::basic_ifstream<char>& input, uint32_t size) :
			palette(size)
		{
			input.read(palette.data(), size);

			lastCRC = CRC::Calculate(typeStr, sizeof(typeStr), crc_table);
			lastCRC = CRC::Calculate(palette.data(), size, crc_table, lastCRC);
		};

		uint32_t getCRC()
		{
			return lastCRC;
		}

		uint32_t lastCRC;
		std::vector<char> palette;
	};


	struct IDAT
	{
		constexpr static ChunkType type = ChunkType::IDAT;
		constexpr static char typeStr[] = { 'I','D','A','T' };

		IDAT() {};

		IDAT(std::basic_ifstream<char>& input, uint32_t size) :
			data(size)
		{
			input.read(data.data(), size);

			lastCRC = CRC::Calculate(typeStr, sizeof(typeStr), crc_table);
			lastCRC = CRC::Calculate(data.data(), size, crc_table, lastCRC);
		};

		void append(std::basic_ifstream<char>& input, uint32_t size)
		{
			data.resize(data.size() + size);
			input.read(data.data() + data.size() - size, size);

			lastCRC = CRC::Calculate(typeStr, sizeof(typeStr), crc_table);
			lastCRC = CRC::Calculate(data.data() + data.size() - size, size, crc_table, lastCRC);
		}

		uint32_t getCRC()
		{
			return lastCRC;
		}

		uint32_t lastCRC;
		std::vector<char> data;
	};


	struct IEND
	{
		constexpr static ChunkType type = ChunkType::IEND;
		constexpr static char typeStr[] = { 'I','E','N','D' };
		IEND(std::basic_ifstream<char>& input, uint32_t size) {};

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
		std::unique_ptr<Chunk<PLTE>> pallette;
		std::unique_ptr<Chunk<IDAT>> image_data;
		std::unique_ptr<Chunk<IEND>> end;
	};

	// Read PNG file
	template <typename T>
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

		return Image<T>();
	}
}
