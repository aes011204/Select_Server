#pragma once

#include <vector>
#include <Windows.h>

using SessionId = UINT64;

struct ReceivedPacket
{
	SessionId Session = 0;
	UINT16 PacketId = 0;

	std::vector<char> Body;
	// 수신 버퍼를 가리키는 포인터가 아니라
	// 이 패킷 객체가 소유하는 데이터
};