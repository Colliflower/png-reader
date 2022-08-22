#include <gtest/gtest.h>

#include <random>

#ifndef TRV_TEST_MULTITHREADED
#undef TRV_PNG_MULTITHREADED
#endif

#include "Zlib.hpp"

using namespace trv;

TEST(TestZlib, TestDeflateNoCompression)
{
	static const std::vector<unsigned char> data { 0x08, 0x1d, 0x01, 0x10, 0x00, 0xef, 0xff,
		                                           0x00, 0x00, 0x00, 0xff, 0x00, 0x0f, 0x00,
		                                           0xf0, 0x00, 0x33, 0x00, 0xcc, 0x00, 0x55,
		                                           0x00, 0xaa, 0x1d, 0x22, 0x03, 0xfd };
	static const std::vector<unsigned char> expected { 0x00, 0x00, 0x00, 0xff, 0x00, 0x0f,
		                                               0x00, 0xf0, 0x00, 0x33, 0x00, 0xcc,
		                                               0x00, 0x55, 0x00, 0xaa };
	std::vector<unsigned char> output;
	DeflateArgs args { true, data, output };

	decompress(args);

	EXPECT_EQ(output.size(), expected.size());

	for (int i = 0; i < expected.size(); ++i)
	{
		EXPECT_EQ(output[i], expected[i]);
	}
}

TEST(TestZlib, PeekLitteByteLittleBit)
{
	static const std::vector<unsigned char> input1 { 0b00100101, 0b01000010 };

	BitConsumer<std::endian::little> consumer(input1);

	std::uint16_t result = consumer.peek_bits<uint16_t, std::endian::little>(1);
	EXPECT_EQ(result, 0b1);

	result = consumer.peek_bits<uint16_t, std::endian::little>(3);
	EXPECT_EQ(result, 0b101);

	result = consumer.peek_bits<uint16_t, std::endian::little>(6);
	EXPECT_EQ(result, 0b100101);

	result = consumer.peek_bits<uint16_t, std::endian::little>(10);
	EXPECT_EQ(result, 0b1000100101);

	result = consumer.peek_bits<uint16_t, std::endian::little>(16);
	EXPECT_EQ(result, 0b0100001000100101);
}

TEST(TestZlib, PeekLitteByteBigBit)
{
	static const std::vector<unsigned char> input1 { 0b00100101, 0b01000010 };

	BitConsumer<std::endian::little> consumer(input1);

	std::uint16_t result = consumer.peek_bits<uint16_t, std::endian::big>(1);
	EXPECT_EQ(result, 0b1);

	result = consumer.peek_bits<uint16_t, std::endian::big>(3);
	EXPECT_EQ(result, 0b101);

	result = consumer.peek_bits<uint16_t, std::endian::big>(6);
	EXPECT_EQ(result, 0b101001);

	result = consumer.peek_bits<uint16_t, std::endian::big>(10);
	EXPECT_EQ(result, 0b1010010001);

	result = consumer.peek_bits<uint16_t, std::endian::big>(16);
	EXPECT_EQ(result, 0b1010010001000010);
}

TEST(TestZlib, PeekBigByteLittleBit)
{
	static const std::vector<unsigned char> input1 { 0b10100100, 0b01000010 };

	BitConsumer<std::endian::big> consumer(input1);

	std::uint16_t result = consumer.peek_bits<uint16_t, std::endian::little>(1);
	EXPECT_EQ(result, 0b1);

	result = consumer.peek_bits<uint16_t, std::endian::little>(3);
	EXPECT_EQ(result, 0b101);

	result = consumer.peek_bits<uint16_t, std::endian::little>(6);
	EXPECT_EQ(result, 0b100101);

	result = consumer.peek_bits<uint16_t, std::endian::little>(10);
	EXPECT_EQ(result, 0b1000100101);

	result = consumer.peek_bits<uint16_t, std::endian::little>(16);
	EXPECT_EQ(result, 0b0100001000100101);
}

TEST(TestZlib, PeekBigByteBigBit)
{
	static const std::vector<unsigned char> input1 { 0b10100100, 0b01000010 };

	BitConsumer<std::endian::big> consumer(input1);

	std::uint16_t result = consumer.peek_bits<uint16_t, std::endian::big>(1);
	EXPECT_EQ(result, 0b1);

	result = consumer.peek_bits<uint16_t, std::endian::big>(3);
	EXPECT_EQ(result, 0b101);

	result = consumer.peek_bits<uint16_t, std::endian::big>(6);
	EXPECT_EQ(result, 0b101001);

	result = consumer.peek_bits<uint16_t, std::endian::big>(10);
	EXPECT_EQ(result, 0b1010010001);

	result = consumer.peek_bits<uint16_t, std::endian::big>(16);
	EXPECT_EQ(result, 0b1010010001000010);
}

TEST(TestZlib, ConsumeLitteByteLittleBit)
{
	static const std::vector<unsigned char> input1 { 0b00110010, 0b00001110, 0b00001111,
		                                             0b00111110 };

	BitConsumer<std::endian::little> consumer(input1);

	std::uint16_t result = consumer.consume_bits<uint16_t, std::endian::little>(2);
	EXPECT_EQ(result, 0b10);

	result = consumer.consume_bits<uint16_t, std::endian::little>(4);
	EXPECT_EQ(result, 0b1100);

	result = consumer.consume_bits<uint16_t, std::endian::little>(6);
	EXPECT_EQ(result, 0b111000);

	result = consumer.consume_bits<uint16_t, std::endian::little>(8);
	EXPECT_EQ(result, 0b11110000);

	result = consumer.consume_bits<uint16_t, std::endian::little>(10);
	EXPECT_EQ(result, 0b1111100000);
}

TEST(TestZlib, ConsumeLitteByteBigBit)
{
	static const std::vector<unsigned char> input1 { 0b00110010, 0b00001110, 0b00001111,
		                                             0b00111110 };

	BitConsumer<std::endian::little> consumer(input1);

	std::uint16_t result = consumer.consume_bits<uint16_t, std::endian::big>(2);
	EXPECT_EQ(result, 0b1);

	result = consumer.consume_bits<uint16_t, std::endian::big>(4);
	EXPECT_EQ(result, 0b11);

	result = consumer.consume_bits<uint16_t, std::endian::big>(6);
	EXPECT_EQ(result, 0b111);

	result = consumer.consume_bits<uint16_t, std::endian::big>(8);
	EXPECT_EQ(result, 0b1111);

	result = consumer.consume_bits<uint16_t, std::endian::big>(10);
	EXPECT_EQ(result, 0b11111);
}

TEST(TestZlib, ConsumeBigByteLittleBit)
{
	static const std::vector<unsigned char> input1 { 0b01001100, 0b01110000, 0b11110000,
		                                             0b01111100 };

	BitConsumer<std::endian::big> consumer(input1);

	std::uint16_t result = consumer.consume_bits<uint16_t, std::endian::little>(2);
	EXPECT_EQ(result, 0b10);

	result = consumer.consume_bits<uint16_t, std::endian::little>(4);
	EXPECT_EQ(result, 0b1100);

	result = consumer.consume_bits<uint16_t, std::endian::little>(6);
	EXPECT_EQ(result, 0b111000);

	result = consumer.consume_bits<uint16_t, std::endian::little>(8);
	EXPECT_EQ(result, 0b11110000);

	result = consumer.consume_bits<uint16_t, std::endian::little>(10);
	EXPECT_EQ(result, 0b1111100000);
}

TEST(TestZlib, ConsumeBigByteBigBit)
{
	static const std::vector<unsigned char> input1 { 0b01001100, 0b01110000, 0b11110000,
		                                             0b01111100 };

	BitConsumer<std::endian::big> consumer(input1);

	std::uint16_t result = consumer.consume_bits<uint16_t, std::endian::big>(2);
	EXPECT_EQ(result, 0b1);

	result = consumer.consume_bits<uint16_t, std::endian::big>(4);
	EXPECT_EQ(result, 0b11);

	result = consumer.consume_bits<uint16_t, std::endian::big>(6);
	EXPECT_EQ(result, 0b111);

	result = consumer.consume_bits<uint16_t, std::endian::big>(8);
	EXPECT_EQ(result, 0b1111);

	result = consumer.consume_bits<uint16_t, std::endian::big>(10);
	EXPECT_EQ(result, 0b11111);
}
