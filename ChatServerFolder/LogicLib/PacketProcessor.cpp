#include "PacketProcessor.h"

#include "../../Common/PacketProtocol.h"
#include "../../Common/PacketID.h"
#include "../../Common/PacketCode.h"
#include <iostream>

NLogicLib::PacketProcessor::PacketProcessor(NServerNetLib::INetwork& network, std::chrono::seconds loginTimeout)
	: m_network(network), m_loginTimeout(loginTimeout),m_rooms(m_users)
{
	if (m_loginTimeout.count() <= 0)
	{
		throw std::invalid_argument("Login timeout must be positive.");
	}

	RegisterHandlers();
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
			HandleConnected(event);
			std::cout << "[Logic] Connected.Session: " << event.Session << "\n";
			break;

		case NServerNetLib::NetworkEventType::Packet:
			// 처리 시점네 미미 종료된 연결이면 무시
			if (false == m_network.IsConnected(event.Session))
				break;
			// 이 확인이 없으면, 늦게 온 요청을 로그인 성공으로 처리하고 대기 목록에서 빼버릴 수 있음
			if (IsLoginExpired(event.Session))
			{
				m_loginDeadlines.erase(event.Session);
				m_network.Disconnect(event.Session);
				break;
			}
			ProcessPacket(event);
			break;
		case NServerNetLib::NetworkEventType::Disconnected:
			HandleDisconnected(event.Session);
			break;
		}
	}
	// 아무 패킷도 보내지 않는 연결도 검사.
	CheckLoginTimeouts();

}



void NLogicLib::PacketProcessor::ProcessPacket(const NServerNetLib::NetworkEvent& event)
{

	const auto index = static_cast<size_t>(event.PacketId);

	if (index >= m_handlers.size())
	{
		std::cerr<< "Packet ID is out of range. Session: "<< event.Session<< ", ID: "<< event.PacketId<< '\n';

		m_network.Disconnect(event.Session);
		return;
	}

	const auto& handler = m_handlers[index];

	if (!handler)
	{
		std::cerr<< "Unsupported packet ID. Session: "<< event.Session<< ", ID: "<< event.PacketId<< '\n';

		m_network.Disconnect(event.Session);
		return;
	}

	handler(event);
}


void NLogicLib::PacketProcessor::HandleLogin(const NServerNetLib::NetworkEvent& event)
{
	Protocol::LoginRequest request;

	if (!Protocol::Decode(event.Body, request))
	{
		m_network.Disconnect(event.Session);
		return;
	}

	const auto result = m_users.Login(event.Session, request.Nickname);

	if (result == Protocol::LoginResult::Success)
	{
		m_loginDeadlines.erase(event.Session);
	}

	Protocol::LoginResponse response;
	response.Result = result;

	const auto body = Protocol::Encode(response);

	if(false == m_network.SendPacket(event.Session, Protocol::LOGIN_RES, body.data(),body.size()))
	{
		m_network.Disconnect(event.Session);
		return;
	}

	if (result == Protocol::LoginResult::Success)
	{
		std::cout << "[Logic] Login sucess.Session: " << event.Session << ", nickname: " << request.Nickname << "\n";
	}
	else
	{
		std::cout << "[Logic] Login rejected .Session: " << event.Session << ", result: " << static_cast<int>(result) << "\n";

	}
}

void NLogicLib::PacketProcessor::HandleDisconnected(SessionId sessionId)
{
	m_loginDeadlines.erase(sessionId);

	const User* user = m_users.Find(sessionId);

	if (user != nullptr)
	{
		// 방정리가 사용자제거보다 우선
		const auto result = m_rooms.LeaveRoom(sessionId);

		if (result != RoomResult::Success && result != RoomResult::NotInRoom)
		{
			std::cerr<< "[Room] Cleanup failed. Session: "<< sessionId<< ", result: "
				<< static_cast<int>(result)<< '\n';
		}

		// Remove() 전에 출력해야 함
		std::cout << "[Logic] Remove user: " << user->Nickname << '\n';
	}
	m_users.Remove(sessionId);

	std::cout << "[Logic] Disconnected. Session: " << sessionId << '\n';
}
void NLogicLib::PacketProcessor::RegisterHandlers()
{
	using Event = NServerNetLib::NetworkEvent;

	RegisterHandler(Protocol::LOGIN_REQ, [this](const Event& event) {HandleLogin(event);});
	RegisterHandler(Protocol::ECHO_REQ, [this](const Event& event) {HandleEcho(event);});
	RegisterHandler(Protocol::CHAT_REQ, [this](const Event& event) {HandleChat(event);});
}

void NLogicLib::PacketProcessor::RegisterHandler(UINT16 packetId, PacketHandler handler)
{
	const auto index =static_cast<size_t>(packetId);

	if (index >= m_handlers.size())
	{
		throw std::out_of_range("Packet handler ID is out of range.");
	}

	if (nullptr==handler)
	{
		throw std::invalid_argument("Packet handler must not be empty.");
	}

	if (m_handlers[index])// 이미 안에 람다가 있다면
	{
		throw std::logic_error(
			"Packet handler is already registered.");
	}

	m_handlers[index] = std::move(handler);

}

void NLogicLib::PacketProcessor::HandleEcho(const NServerNetLib::NetworkEvent& event)
{
	// 에코는 연결 점검용이므로 로그인 전에도 허용
	const bool queued = m_network.SendPacket(event.Session, Protocol::ECHO_RES, event.Body.data(), event.Body.size());
	if (!queued)
	{
		m_network.Disconnect(event.Session);
	}
}

void NLogicLib::PacketProcessor::HandleConnected(const NServerNetLib::NetworkEvent& event)
{
	//새 연결의 마감 시각 등록
	
	// 처리 전에 이미 연결이 끝났을 수 있다.
	if (!m_network.IsConnected(event.Session))
		return;

	m_loginDeadlines[event.Session] = event.OccurredAt + m_loginTimeout;

	std::cout << "[Logic] Connected. Session: " << event.Session << '\n';
}

bool NLogicLib::PacketProcessor::IsLoginExpired(SessionId sessionId) const
{
	//특정 연결 한 명의 만료 여부 확인
	const auto it = m_loginDeadlines.find(sessionId);

	if (it == m_loginDeadlines.end())
		return false;

	return Clock::now() >= it->second;
}

void NLogicLib::PacketProcessor::CheckLoginTimeouts()
{
	//전체 대기 연결을 검사하고 만료된 연결 종료

	const auto now = Clock::now();

	for (auto it = m_loginDeadlines.begin();
		it != m_loginDeadlines.end();)
	{
		if (now < it->second)
		{
			++it;
			continue;
		}

		const SessionId sessionId = it->first;

		// 먼저 대기 목록에서 제거한다.
		it = m_loginDeadlines.erase(it);

		std::cout
			<< "[Logic] Login timeout. Session: "
			<< sessionId
			<< '\n';

		m_network.Disconnect(sessionId);
	}
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
