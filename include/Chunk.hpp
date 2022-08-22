#pragma once

#include <cstddef>
#include <iostream>
#include <memory>
#include <sstream>
#include <type_traits>

#include "CRC.hpp"

namespace trv
{
inline constexpr CRCTable CRC32Table {};
// Enum of supported chunk types
enum class ChunkType : std::size_t
{
	IHDR,
	PLTE,
	IDAT,
	IEND,
	Count,
	Unknown
};

// Chunk data type, has requirements for generic construction
template <typename T>
concept IsChunk = requires(T x)
{
	requires std::is_constructible_v < T, std::basic_ifstream<char>
	&, std::uint32_t > ;
};

// Generic container for chunk in PNG with auxiliary data
template <IsChunk T>
struct Chunk
{
	Chunk(std::basic_ifstream<char>& input, std::uint32_t size, std::uint32_t type) :
	    size(size), type(type), data(input, size), crc(extract_from_ifstream<uint32_t>(input))
	{
		std::uint32_t computed_crc = data.getCRC();
		if (computed_crc != crc)
		{
			std::stringstream msg;
			msg << "Computed CRC 0x" << std::uppercase << std::setfill('0') << std::setw(8)
			    << std::hex << computed_crc << " doesn't match read CRC 0x" << std::uppercase
			    << std::setfill('0') << std::setw(8) << std::hex << crc;
			throw std::runtime_error(msg.str());
		}
	};

	void append(std::basic_ifstream<char>& input, std::uint32_t chunkSize)
	{
		data.append(input, chunkSize);
		std::uint32_t file_crc     = extract_from_ifstream<uint32_t>(input);
		std::uint32_t computed_crc = data.getCRC();

		if (computed_crc != file_crc)
		{
			std::stringstream msg;
			msg << "Computed CRC "
			    << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex
			    << computed_crc << " doesn't match read CRC "
			    << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex
			    << file_crc;
			throw std::runtime_error(msg.str());
		}
	}

	std::uint32_t size;
	std::uint32_t type;
	T data;
	std::uint32_t crc;
};

struct IHDR
{
   public:
	constexpr static ChunkType type = ChunkType::IHDR;
	constexpr static char typeStr[] = { 'I', 'H', 'D', 'R' };

	IHDR() = default;
	IHDR(std::basic_ifstream<char>& input, std::uint32_t)
	{
		width             = extract_from_ifstream<uint32_t>(input);
		height            = extract_from_ifstream<uint32_t>(input);
		bitDepth          = extract_from_ifstream<uint8_t>(input);
		colorType         = extract_from_ifstream<uint8_t>(input);
		compressionMethod = extract_from_ifstream<uint8_t>(input);
		filterMethod      = extract_from_ifstream<uint8_t>(input);
		interlaceMethod   = extract_from_ifstream<uint8_t>(input);

		verify();

		lastCRC            = CRC32Table.crc(typeStr, sizeof(typeStr));
		std::uint32_t temp = big_endian<uint32_t>(width);
		lastCRC            = CRC32Table.crc(lastCRC, &temp, sizeof(temp));
		temp               = big_endian<uint32_t>(height);
		lastCRC            = CRC32Table.crc(lastCRC, &temp, sizeof(temp));
		lastCRC            = CRC32Table.crc(lastCRC, &bitDepth, 5);
	};

	[[nodiscard]] std::uint32_t getCRC() { return lastCRC; }

   private:
	void verify()
	{
		if (width == 0 || height == 0)
		{
			throw std::runtime_error("TRV::CHUNK::IHDR Invalid width or height of zero detected.");
		}

		switch (bitDepth)
		{
			case 1:
			case 2:
			case 4:
			case 8:
			case 16:
				break;
			default:
				throw std::runtime_error("TRV::CHUNK::IHDR Invalid bit depth detected.");
		}

		switch (colorType)
		{
			case 0:
			case 2:
				if (bitDepth < 8)
				{
					throw std::runtime_error(
					    "TRV::CHUNK::IHDR Invalid bit depth detected for color type 2.");
				}
				break;
			case 3:
				if (bitDepth > 8)
				{
					throw std::runtime_error(
					    "TRV::CHUNK::IHDR Invalid bit depth detected for color type 3.");
				}
				break;
			case 4:
				if (bitDepth < 8)
				{
					throw std::runtime_error(
					    "TRV::CHUNK::IHDR Invalid bit depth detected for color type 4.");
				}
				break;
			case 6:
				if (bitDepth < 8)
				{
					throw std::runtime_error(
					    "TRV::CHUNK::IHDR Invalid bit depth detected for color Type 6.");
				}
				break;
			default:
				throw std::runtime_error("TRV::CHUNK::IHDR Invalid color type detected.");
		}

		if (compressionMethod != 0)
		{
			throw std::runtime_error("TRV::CHUNK::IHDR Invalid compression method detected.");
		}

		if (filterMethod != 0)
		{
			throw std::runtime_error("TRV::CHUNK::IHDR Invalid filter method detected.");
		}

		if (interlaceMethod > 1)
		{
			throw std::runtime_error("TRV::CHUNK::IHDR Invalid interlace method detected.");
		}
	}

   public:
	std::uint32_t lastCRC;
	std::uint32_t width;
	std::uint32_t height;
	std::uint8_t bitDepth;
	std::uint8_t colorType;
	std::uint8_t compressionMethod;
	std::uint8_t filterMethod;
	std::uint8_t interlaceMethod;
};

struct PLTE
{
   public:
	constexpr static ChunkType type = ChunkType::PLTE;
	constexpr static char typeStr[] = { 'P', 'L', 'T', 'E' };

	PLTE() = default;
	PLTE(std::basic_ifstream<char>& input, std::uint32_t size) : data(size)
	{
		input.read(reinterpret_cast<char*>(data.data()), size);

		verify();

		lastCRC = CRC32Table.crc(typeStr, sizeof(typeStr));
		lastCRC = CRC32Table.crc(lastCRC, data.data(), size);
	};

	[[nodiscard]] std::uint32_t getCRC() { return lastCRC; }

   private:
	void verify()
	{
		if (data.size() % 3 != 0)
		{
			throw std::runtime_error("TRV::CHUNK::PLTE Palette entries must be a multiple of 3.");
		}
	}

   public:
	std::uint32_t lastCRC;
	std::vector<unsigned char> data;
};

struct IDAT
{
	constexpr static ChunkType type = ChunkType::IDAT;
	constexpr static char typeStr[] = { 'I', 'D', 'A', 'T' };

	IDAT() = default;
	IDAT(std::basic_ifstream<char>& input, std::uint32_t size) : lastCRC(), data(size)
	{
		input.read(reinterpret_cast<char*>(data.data()), size);

		lastCRC = CRC32Table.crc(typeStr, sizeof(typeStr));
		lastCRC = CRC32Table.crc(lastCRC, data.data(), size);
	};

	void append(std::basic_ifstream<char>& input, std::uint32_t size)
	{
		data.resize(data.size() + size);
		input.read(reinterpret_cast<char*>(data.data() + data.size() - size), size);

		lastCRC = CRC32Table.crc(typeStr, sizeof(typeStr));
		lastCRC = CRC32Table.crc(lastCRC, data.data() + data.size() - size, size);
	}

	[[nodiscard]] std::uint32_t getCRC() { return lastCRC; }

	std::uint32_t lastCRC;
	std::vector<unsigned char> data;
};

struct IEND
{
	constexpr static ChunkType type = ChunkType::IEND;
	constexpr static char typeStr[] = { 'I', 'E', 'N', 'D' };
	IEND()                          = default;
	IEND(std::basic_ifstream<char>&, std::uint32_t) {};

	[[nodiscard]] constexpr std::uint32_t getCRC() const { return 0xAE426082; }
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

void verifyOrdering(const Chunk<IHDR>* header, const std::vector<ChunkType>& sequence);
}  // namespace trv
