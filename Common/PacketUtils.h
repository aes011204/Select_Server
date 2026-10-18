#pragma once
#include <windows.h>
namespace Protocol
{

	inline UINT16 ReadUInt16(const char* data)
	{
		const auto high = static_cast<unsigned char>(data[0]);
		const auto low = static_cast<unsigned char>(data[1]);

		return (static_cast<unsigned int>(static_cast<unsigned int>(high) << 8) | low);
	}

	inline void WriteUInt16(char* data, UINT16 value)
	{
		auto* bytes = reinterpret_cast<unsigned char*>(data);

		bytes[0] = static_cast<unsigned char>((value >> 8) & 0xFF);

		bytes[1] = static_cast<unsigned char>(value & 0xFF);

	}
}