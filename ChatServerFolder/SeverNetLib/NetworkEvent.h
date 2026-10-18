#pragma once
#include <vector>
#include <Windows.h>
#include <chrono>

namespace NServerNetLib
{

	using SessionId = UINT64;


	enum class NetworkEventType
	{
		Connected,
		Packet,
		Disconnected
	};

	struct NetworkEvent
	{
		NetworkEventType type = NetworkEventType::Packet;
		SessionId Session = 0;

		//패킷 이벤트일떄 사용
		UINT16 PacketId = 0;
		std::vector<char> Body;
		// 수신 버퍼를 가리키는 포인터가 아니라
		// 이 패킷 객체가 소유하는 데이터

		std::chrono::steady_clock::time_point OccurredAt =
			std::chrono::steady_clock::now();
	};

}