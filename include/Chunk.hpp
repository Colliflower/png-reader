#pragma once

#include <cstddef>
#include <iostream>
#include <memory>
#include <type_traits>

#include "CRC.hpp"

namespace trv
{
static constexpr CRCTable CRC32Table {};
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
			msg << "Computed CRC "
			    << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex
			    << computed_crc << " doesn't match read CRC "
			    << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << crc;
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

		lastCRC            = CRC32Table.crc(typeStr, sizeof(typeStr));
		std::uint32_t temp = big_endian<uint32_t>(width);
		lastCRC            = CRC32Table.crc(lastCRC, &temp, sizeof(temp));
		temp               = big_endian<uint32_t>(height);
		lastCRC            = CRC32Table.crc(lastCRC, &temp, sizeof(temp));
		lastCRC            = CRC32Table.crc(lastCRC, &bitDepth, 5);
	};

	std::uint32_t getCRC() { return lastCRC; }

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
	constexpr static ChunkType type = ChunkType::PLTE;
	constexpr static char typeStr[] = { 'P', 'L', 'T', 'E' };

	PLTE(std::basic_ifstream<char>& input, std::uint32_t size) : data(size)
	{
		input.read(reinterpret_cast<char*>(data.data()), size);

		lastCRC = CRC32Table.crc(typeStr, sizeof(typeStr));
		lastCRC = CRC32Table.crc(lastCRC, data.data(), size);
	};

	std::uint32_t getCRC() { return lastCRC; }

	std::uint32_t lastCRC;
	std::vector<unsigned char> data;
};

struct IDAT
{
	constexpr static ChunkType type = ChunkType::IDAT;
	constexpr static char typeStr[] = { 'I', 'D', 'A', 'T' };

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

	std::uint32_t getCRC() { return lastCRC; }

	std::uint32_t lastCRC;
	std::vector<unsigned char> data;
};

struct IEND
{
	constexpr static ChunkType type = ChunkType::IEND;
	constexpr static char typeStr[] = { 'I', 'E', 'N', 'D' };
	IEND(std::basic_ifstream<char>&, std::uint32_t) {};

	constexpr std::uint32_t getCRC() const { return 0xAE426082; }
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
}