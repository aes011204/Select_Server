#include "../SeverNetLib/TcpNetwork.h"
#include "PacketProcessor.h"

#include "../../Common/PacketProtocol.h"
#include "../../Common/PacketID.h"

#include <iostream>

NLogicLib::PacketProcessor::PacketProcessor(NServerNetLib::TcpNetwork& network): m_network(network)
{
}

void NLogicLib::PacketProcessor::Update()
{
	NServerNetLib::NetworkEvent event;

	while (m_network.TryPopEvent(event))
	{
		// 수신 처리저네 연결이 종료 되었을 수 있음
		switch (event.type)
		{
		case NServerNetLib::NetworkEventType::Connected:
			std::cout << "[Logic] Connected.Session: " << event.Session << "\n";
			break;

		case NServerNetLib::NetworkEventType::Packet:
			// 처리 시점네 미미 종료된 연결이면 무시
			if (m_network.IsConnected(event.Session))
			{
				ProcessPacket(event);
			}
			break;
		case NServerNetLib::NetworkEventType::Disconnected:
			HandleDisconnected(event.Session);
			break;
		}
	}
}

void NLogicLib::PacketProcessor::ProcessPacket(const NServerNetLib::NetworkEvent& event)
{
	switch (event.PacketId)
	{
	case Protocol::LOGIN_REQ:
		HandleLogin(event);
		break;
	case Protocol::ECHO_REQ:
	{
		// 에코는 연결 점검용이므로 로그인 전에도 허용
		const bool queued = m_network.SendPacket(event.Session, Protocol::ECHO_RES, event.Body.data(), event.Body.size());
		if (!queued)
		{
			m_network.Disconnect(event.Session);
		}
		break;
	}
	case Protocol::CHAT_REQ:
		HandleChat(event);
		break;
	default:
		std::cerr << "Unknown packet ID: " << event.PacketId << "\n";
		m_network.Disconnect(event.Session);
		break;
	}


}

void NLogicLib::PacketProcessor::HandleLogin(const NServerNetLib::NetworkEvent& event)
{
	const std::string nickname(event.Body.begin(),event.Body.end());

	const Protocol::LoginResult result = m_users.Login(event.Session, nickname);

	// 응답 본문은 결과 토드 한 바이트
	const char response = static_cast<char>(result);

	const bool queued = m_network.SendPacket(event.Session, Protocol::LOGIN_RES, &response, 1);

	if (!queued)
	{
		m_network.Disconnect(event.Session);
		return;
	}

	if (result == Protocol::LoginResult::Success)
	{
		std::cout << "[Logic] Login sucess.Session: " << event.Session << ", nickname: " << nickname << "\n";
	}
	else
	{
		std::cout << "[Logic] Login rejected .Session: " << event.Session << ", result: " << static_cast<int>(result) << "\n";

	}
}

void NLogicLib::PacketProcessor::HandleDisconnected(SessionId sessionId)
{
	const User* user = m_users.Find(sessionId);

	if (user != nullptr)
	{
		// Remove() 전에 출력해야 함
		std::cout << "[Logic] Remove user: " << user->Nickname << '\n';
	}
	m_users.Remove(sessionId);

	std::cout << "[Logic] Disconnected. Session: " << sessionId << '\n';
}



//void NLogicLib::PacketProcessor::Process(const ReceivedPacket& packet)
//{
//	std::cout << "[Logic] Session: "
//		<< packet.Session
//		<< ", packet ID: "
//		<< packet.PacketId
//		<< '\n';
//
//	switch (packet.PacketId)
//	{
//	case Protocol::ECHO_REQ:
//	{
//		const bool queued = m_network.SendPacket(packet.Session, Protocol::ECHO_RES, packet.Body.data(), packet.Body.size());
//
//		if (!queued)
//		{
//			std::cerr << "Could not queue echo response.\n";
//			m_network.Disconnect(packet.Session);
//		}
//		break;
//	}
//	default:
//		std::cerr << "Unknown packet ID: " << packet.PacketId << "\n";
//
//		m_network.Disconnect(packet.Session);
//		break;
//
//	}
//}
