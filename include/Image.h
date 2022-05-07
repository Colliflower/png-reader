#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <bit>
#include <type_traits>
#include <memory>
#include <assert.h>

namespace trv
{
	//unsigned long long header = 0x0a1a0a0d474e5089;
	unsigned long long header = 0x89504e470d0a1a0a;

	template <typename T>
	struct Image
	{
		std::vector<T> data;
		unsigned int width, height, channels;
	};

	template <typename T>
	T correct_endian(const char*);

	enum class ChunkType : size_t { IHDR, PLTE, IDAT, IEND, Count };

	template<typename T>
	concept IsChunk = requires (T x)
	{
		//{x.size} -> std::same_as<size_t>;

		requires std::is_constructible_v<T, const char*, size_t>;

		requires std::is_default_constructible_v<T>;
	};

	template <IsChunk T>
	struct Chunk
	{
		Chunk() : size(), type(), data(), crc() {};
		Chunk(const char* buffer, size_t size) : size(correct_endian<uint32_t>(buffer)),
			type(*(uint32_t*)(buffer + sizeof(uint32_t))),
			data(buffer + sizeof(uint32_t) * 2, size),
			crc(correct_endian<uint32_t>(buffer + data.size))
		{};

		uint32_t size;
		uint32_t type;
		T data;
		uint32_t crc;
	};
	
	struct IHDR
	{
		constexpr static size_t size = sizeof(uint32_t) * 2 + sizeof(char) * 5;

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
		IEND() = default;

		IEND(const char* buffer, size_t size) {};
		constexpr static size_t size = 0;
	};

	struct Chunks
	{
		Chunks() {};
		std::unique_ptr<Chunk<IHDR>> header;
		std::unique_ptr<Chunk<PLTE>> pallette;
		std::vector<Chunk<IDAT>> image_data;
		std::unique_ptr<Chunk<IEND>> end;
	};

	template <typename T>
	T correct_endian(const char* buffer) {
		T val = *(T*)buffer;

		if (std::endian::native == std::endian::little)
		{
			val = std::byteswap<T>(val);
		}

		return val;
	}
	

	constexpr unsigned long encode_type_impl(const char* str, unsigned long val = 0)
	{
		return *str ?
			(val ?
				encode_type_impl(str + 1, *str) << 8 | val
				: encode_type_impl(str + 1, *str))
			: val;
	}

	constexpr unsigned long encode_type(const char* str)
	{
		return encode_type_impl(str);
	}



	template <typename T>
	Image<T> load_image(const std::string& path)
	{
		std::ifstream infile(path, std::ios_base::binary);

		std::vector<char> buffer{ std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>() };

		const char* head = buffer.data();

		unsigned long long file_header = correct_endian<unsigned long long>(head);
		head += sizeof(header);

		if (file_header != header)
		{
			throw std::runtime_error("Unable to read file, invalid PNG header.");
		}

		unsigned long size = correct_endian<unsigned long>(head);
		head += sizeof(size);
		unsigned long type = *(unsigned long*)head;
		head -= sizeof(type);
		
		constexpr unsigned long ihdr = encode_type("IHDR");
		if (type != ihdr)
		{
			throw std::runtime_error("Unable to parse PNG, IHDR chunk missing.");
		}

		Chunks chunks;
		chunks.header = std::make_unique<Chunk<IHDR>>(head, size);
	}
}