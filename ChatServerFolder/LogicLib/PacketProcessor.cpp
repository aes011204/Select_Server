#include "../SeverNetLib/TcpNetwork.h"
#include "../SeverNetLib/ReceivedPacket.h"
#include "PacketProcessor.h"

#include "../../Common/PacketProtocol.h"
#include "../../Common/PacketID.h"

#include <iostream>

NLogicLib::PacketProcessor::PacketProcessor(NServerNetLib::TcpNetwork& network): m_network(network)
{
}

void NLogicLib::PacketProcessor::Update()
{
	ReceivedPacket packet;

	while (m_network.TryPopPacket(packet))
	{
		// 수신 처리저네 연결이 종료 되었을 수 있음
		if (!m_network.IsConnected(packet.Session))
		{
			std::cout << "Skip stale packet. Session: "
				<< packet.Session << '\n';

			continue;
		}

		Process(packet);
	}
}

void NLogicLib::PacketProcessor::Process(const ReceivedPacket& packet)
{
	std::cout << "[Logic] Session: "
		<< packet.Session
		<< ", packet ID: "
		<< packet.PacketId
		<< '\n';

	switch (packet.PacketId)
	{
	case Protocol::ECHO_REQ:
	{
		const bool queued = m_network.SendPacket(packet.Session, Protocol::ECHO_RES, packet.Body.data(), packet.Body.size());

		if (!queued)
		{
			std::cerr << "Could not queue echo response.\n";
			m_network.Disconnect(packet.Session);
		}
		break;
	}
	default:
		std::cerr << "Unknown packet ID: " << packet.PacketId << "\n";

		m_network.Disconnect(packet.Session);
		break;

	}
}
