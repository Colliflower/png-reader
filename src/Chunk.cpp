#include "Chunk.hpp"

#include <algorithm>

namespace trv
{
void verifyOrdering(const Chunk<IHDR>* header, const std::vector<ChunkType>& sequence)
{
	std::array<std::size_t, static_cast<std::size_t>(ChunkType::Count)> previousPosition {};

	for (std::size_t i = 0; i < sequence.size(); ++i)
	{
		const auto& curr = sequence[i];

		if (curr == ChunkType::Unknown)
		{
			continue;
		}

		if (i == 0 && curr != ChunkType::IHDR)
		{
			throw std::runtime_error(
			    "TRV::PNG::CHUNK Invalid chunk sequence IDHR must appear first.");
		}

		std::size_t chunk = static_cast<std::size_t>(curr);

		if (previousPosition[chunk] != 0 && curr != ChunkType::IDAT)
		{
			throw std::runtime_error(
			    "TRV::PNG::CHUNK Chunk has unexpectedly appeared multiple times.");
		}

		if (header->data.colorType == 3 && curr == ChunkType::IDAT &&
		    previousPosition[static_cast<std::size_t>(ChunkType::PLTE)] == 0)
		{
			throw std::runtime_error(
			    "TRV::PNG::CHUNK PLTE must be provided before IDAT with color type 3.");
		}

		if (curr == ChunkType::IDAT && previousPosition[chunk] > 0 &&
		    previousPosition[chunk] != i - 1)
		{
			throw std::runtime_error("TRV::PNG::CHUNK IDAT sequences must appear consecutively.");
		}

		if ((curr == ChunkType::IEND && i != sequence.size() - 1) ||
		    (curr != ChunkType::IEND && i == sequence.size() - 1))
		{
			throw std::runtime_error("TRV::PNG::CHUNK IEND must be the final chunk.");
		}

		previousPosition[chunk] = i;
	}
}
}
