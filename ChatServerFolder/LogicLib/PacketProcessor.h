#pragma once
#include "../SeverNetLib/NetworkEvent.h"
#include "UserManager.h"

namespace NServerNetLib
{
	class TcpNetwork;
}

namespace NLogicLib
{


	class PacketProcessor
	{
	public:
		PacketProcessor(NServerNetLib::TcpNetwork& network);

		void Update();
	private:
		//void Process(const ReceivedPacket& packet);

		void ProcessPacket(const NServerNetLib::NetworkEvent& event);
		void HandleLogin(const NServerNetLib::NetworkEvent& event);
		void HandleDisconnected(SessionId sessionId);

		void HandleChat(const NServerNetLib::NetworkEvent& event);
		bool SendChatResult(SessionId sessionId, Protocol::ChatResult result);

		bool IsValidChatMessage(const std::string& message) const;
	private:

		NServerNetLib::TcpNetwork& m_network;
		UserManager m_users;


	};

}