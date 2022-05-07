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
	uint64_t header = 0x89504e470d0a1a0a;

	// Output type
	template <typename T>
	struct Image
	{
		std::vector<T> data;
		uint32_t width, height, channels;
	};


	// Allows reading of integral types from big-endian binary stream
	template <std::integral T>
	T correct_endian(const char* buffer) {
		T val = *(T*)buffer;

		if (std::endian::native == std::endian::little)
		{
			val = std::byteswap<T>(val);
		}

		return val;
	}

	// Enum of supported chunk types
	enum class ChunkType : size_t { IHDR, PLTE, IDAT, IEND, Count };

	// Chunk data type, has requirements for generic construction
	template<typename T>
	concept IsChunk = requires (T x)
	{
		requires std::is_constructible_v<T, const char*, size_t>;

		requires std::is_default_constructible_v<T>;
	};

	// Generic container for chunk in PNG with auxilliary data
	template <IsChunk T>
	struct Chunk
	{
		Chunk() : size(), type(), data(), crc() {};
		Chunk(const char* buffer, size_t size) : size(correct_endian<uint32_t>(buffer)),
			type(*(uint32_t*)(buffer + sizeof(uint32_t))),
			data(buffer + sizeof(uint32_t) * 2, size),
			crc(correct_endian<uint32_t>(buffer + +sizeof(uint32_t)*2 + data.size))
		{
			uint32_t computed_crc = CRC::Calculate(buffer + sizeof(uint32_t), sizeof(uint32_t) + size, CRC::CRC_32());

			if (computed_crc != crc)
			{
				std::stringstream msg;
				msg << "Computed CRC " << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << computed_crc
					<< " doesn't match read CRC " << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << crc;
				throw std::runtime_error(msg.str());
			}
		};

		uint32_t size;
		uint32_t type;
		T data;
		uint32_t crc;
	};
	

	struct IHDR
	{
		constexpr static size_t size = sizeof(uint32_t) * 2 + sizeof(char) * 5;
		constexpr static ChunkType type = ChunkType::IHDR;

		IHDR() = default;

		IHDR(const char* buffer, size_t size)
		{
			width = correct_endian<uint32_t>(buffer);
			height = correct_endian<uint32_t>(buffer + sizeof(uint32_t));
			bitDepth = correct_endian<char>(buffer + sizeof(uint32_t) * 2);
			colorType = correct_endian<char>(buffer + sizeof(uint32_t) * 2 + sizeof(char));
			compressionMethod = correct_endian<char>(buffer + sizeof(uint32_t) * 2 + sizeof(char) * 2);
			filterMethod = correct_endian<char>(buffer + sizeof(uint32_t) * 2 + sizeof(char) * 3);
			interlaceMethod = correct_endian<char>(buffer + sizeof(uint32_t) * 2 + sizeof(char) * 4);
		};

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
		PLTE() : size(), pallette() {};

		PLTE(const char* buffer, size_t size) :
			size(size),
			pallette(buffer, buffer + size)
		{};
		size_t size;
		std::vector<char> pallette;
	};


	struct IDAT
	{
		constexpr static ChunkType type = ChunkType::IDAT;
		IDAT() : size(), data() {};

		IDAT(const char* buffer, size_t size) :
			size(size),
			data(buffer, buffer + size)
		{};
		size_t size;
		std::vector<char> data;
	};


	struct IEND
	{
		constexpr static ChunkType type = ChunkType::IEND;
		IEND() = default;

		IEND(const char* buffer, size_t size) {};
		constexpr static size_t size = 0;
	};

	// Container for supported chunk types
	struct Chunks
	{
		Chunks() {};
		std::unique_ptr<Chunk<IHDR>> header;
		std::unique_ptr<Chunk<PLTE>> pallette;
		std::vector<Chunk<IDAT>> image_data;
		std::unique_ptr<Chunk<IEND>> end;
	};
	
	// Recursive compile-time encoding of chunk types
	constexpr uint32_t encode_type_impl(const char* str, uint32_t val = 0)
	{
		return *str ?
			(val ?
				encode_type_impl(str + 1, *str) << 8 | val
				: encode_type_impl(str + 1, *str))
			: val;
	}

	// Compile-time encoding of chunk types
	constexpr uint32_t encode_type(const char* str)
	{
		return encode_type_impl(str);
	}

	// Read PNG file
	template <typename T>
	Image<T> load_image(const std::string& path)
	{
		std::ifstream infile(path, std::ios_base::binary);

		std::vector<char> buffer{ std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>() };

		const char* head = buffer.data();

		uint64_t file_header = correct_endian<uint64_t>(head);
		head += sizeof(header);

		if (file_header != header)
		{
			throw std::runtime_error("Unable to read file, invalid PNG header.");
		}

		uint32_t size = correct_endian<uint32_t>(head);
		head += sizeof(size);
		uint32_t type = *(uint32_t*)head;
		head -= sizeof(type);
		
		constexpr uint32_t ihdr = encode_type("IHDR");
		if (type != ihdr)
		{
			throw std::runtime_error("Unable to parse PNG, IHDR chunk missing.");
		}

		Chunks chunks;
		chunks.header = std::make_unique<Chunk<IHDR>>(head, size);

		return Image<T>();
	}
}
