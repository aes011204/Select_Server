#pragma once

#include <winsock2.h>
#include <deque>
#include <vector>
#include "ServerNetErrorCode.h"
#include "INetwork.h"
#include "NetworkConfig.h"
#include "../../Common/PacketProtocol.h"
//#include "ReceivedPacket.h"
#include "NetworkEvent.h"

using SessionId = UINT64;

namespace NServerNetLib
{

	struct ClientSession
	{
		SessionId Id = 0;
		//클라이언트마다소켓 번호뿐 아니라 버퍼 같은 정보도 함께 관리하기 위해
		SOCKET Socket = INVALID_SOCKET;
		std::vector<char> RecvBuffer;
		std::vector<char> SendBuffer;
	};


	class TcpNetwork : public INetwork
	{
	public:
		TcpNetwork();
		~TcpNetwork();

		TcpNetwork(const TcpNetwork&) = delete;
		TcpNetwork& operator= (const TcpNetwork&) = delete;

	public:
		NET_ERROR_CODE Init(const NetworkConfig& config);
		bool Run();
		void Release();


		bool TryPopEvent(NetworkEvent& outEvent)override;
		bool IsConnected(SessionId sessionId) const override;
		bool SendPacket(SessionId sessionId, UINT16 packetId, const char* body, size_t bodySize) override;
		void Disconnect(SessionId sessionId) override;

	private:
		NET_ERROR_CODE SetNonBlockSocket(const SOCKET sock);

		NET_ERROR_CODE AcceptClient();

		bool ReceiveClient(ClientSession& client);
		bool SendClient(ClientSession& client);

		bool ProcessRecvBuffer(ClientSession& client);
		//bool HandlePacket(ClientSession& client, UINT16 packetId, const char* body, size_t bodysize);
		bool EnqueueReceivedPacket(ClientSession& client, UINT16 packetId, const char* body, size_t bodysize);
		bool QueuePacket(ClientSession& client, UINT16 packetId, const char* body, size_t bodysize);

		void CloseClient(size_t index);

		void PushConnectionEvent(NetworkEventType type, SessionId sessionId);

	private:
		NetworkConfig m_config{};

		SOCKET m_listenSocket = INVALID_SOCKET;
		bool m_winsockStarted = false;

		std::vector<ClientSession> m_clients;
		std::deque<NetworkEvent> m_events;

		SessionId m_lastSessionId = 0; 


	};

}