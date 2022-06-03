#pragma once

#include <concepts>
#include <array>
#include "Common.h"

namespace trv
{

	template <std::unsigned_integral T>
	struct CRCTable
	{
		template <std::unsigned_integral T>
		struct Model
		{
			T polynomial;
			T initialValue;
			bool inputReflected;
			bool resultReflected;
			T finalXOR;
		};

		static constexpr Model<T> CRC_32 = { 0x04C11DB7, 0XFFFFFFFF, true, true, 0xFFFFFFFF };


		constexpr CRCTable(const Model<T>& model) : lookupTable(), model(model)
		{
			for (T dividend = 0; dividend < 256; ++dividend)
			{
				constexpr int size = sizeof(T) * 8;
				T curr = dividend << (size - 8); // Shift into MSB

				for (int bit = 0; bit < 8; ++bit)
				{

					if (curr & (1 << (size - 1)))
					{
						curr <<= 1;
						curr ^= model.polynomial;
					}
					else
					{
						curr <<= 1;
					}
				}

				lookupTable[dividend] = curr;
			}
		};

		template <std::integral U>
		T Calculate(U value, T previous) const
		{
			previous ^= model.finalXOR;

			for (int byte = sizeof(U) - 1; byte >= 0; --byte)
			{
				T current = (value & (((1 << 8) - 1) << byte * 8)) >> (byte * 8);

				if (model.inputReflected)
				{
					current = reverse_byte(static_cast<uint8_t>(current));
				}
				previous = (previous ^ (current << (sizeof(T) - 1) * 8));
				uint8_t pos = static_cast<uint8_t>(previous >> ((sizeof(T) - 1) * 8));
				previous = (previous << 8) ^ lookupTable[pos];
			}

			previous = (model.resultReflected ? reverse_bits<T>(previous) : previous);

			return previous ^ model.finalXOR;
		}

		template <std::integral U>
		T Calculate(U value) const
		{
			return Calculate<U>(value, 0);
		}

		template <std::integral U, size_t N>
		T Calculate(const U (&values)[N], T previous) const
		{
			for (const U& value : values)
			{
				previous = Calculate<U>(value, previous);
			}

			return previous;
		}

		template <std::integral U, size_t N>
		T Calculate(const U(&values)[N]) const
		{
			T previous = model.initialValue;

			for (const U& value : values)
			{
				previous = Calculate<U>(value, previous);
			}

			return previous;
		}

		template <std::integral U>
		T Calculate(const U* values, size_t num, T previous) const
		{
			for (size_t i = 0; i < num; ++i)
			{
				previous = Calculate<U>(values[i], previous);
			}

			return previous;
		}

		template <std::integral U>
		T Calculate(const U* values, size_t num) const
		{
			return Calcualte<U>(values, num, 0);
		}

		Model<T> model;
		std::array<T, 256> lookupTable;
	};
}