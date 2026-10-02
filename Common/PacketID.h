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

	constexpr UINT16 ROOM_CREATE_REQ = 20;
	constexpr UINT16 ROOM_CREATE_RES = 21;

	constexpr UINT16 ROOM_LIST_REQ = 22;
	constexpr UINT16 ROOM_LIST_RES = 23;

	constexpr UINT16 ROOM_ENTER_REQ = 24;
	constexpr UINT16 ROOM_ENTER_RES = 25;

	constexpr UINT16 ROOM_LEAVE_REQ = 26;
	constexpr UINT16 ROOM_LEAVE_RES = 27;

	constexpr UINT16 ROOM_MEMBER_NTF = 28;

}