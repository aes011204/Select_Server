#pragma once
#include <Windows.h>

namespace Protocol
{
	constexpr UINT16 LOGIN_REQ = 1;
	constexpr UINT16 LOGIN_RES = 2;
	constexpr UINT16 ECHO_REQ = 3;
	constexpr UINT16 ECHO_RES = 4;
	constexpr UINT16 CHAT_REQ = 5;
	constexpr UINT16 CHAT_RES = 6;
	constexpr UINT16 CHAT_NTF = 7;

}