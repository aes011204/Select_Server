#pragma once

#include <winsock2.h>
#include <deque>
#include <vector>
#include "ServerNetErrorCode.h"

#include "../../Common/PacketProtocol.h"
#include "ReceivedPacket.h"

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


	class TcpNetwork
	{
	public:
		TcpNetwork();
		~TcpNetwork();

		TcpNetwork(const TcpNetwork&) = delete;
		TcpNetwork& operator= (const TcpNetwork&) = delete;

	public:
		NET_ERROR_CODE Init(UINT16 port);
		bool Run();
		void Release();


		bool TryPopPacket(ReceivedPacket& outPacket);
		bool IsConnected(SessionId sessionId) const;
		bool SendPacket(SessionId sessionId, UINT16 packetId, const char* body, size_t bodySize);
		void Disconnect(SessionId sessionId);

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

	private:
		SOCKET m_listenSocket = INVALID_SOCKET;
		bool m_winsockStarted = false;

		std::vector<ClientSession> m_clients;
		std::deque<ReceivedPacket> m_receivedPackets;

		SessionId m_lastSessionId = 0; 

		// 읽기 집합에서 리스닝 소켓 한자리를 제외
		static constexpr size_t MAX_CLIENTS = FD_SETSIZE - 1;

		// 클라이언트 별 송산 대기 데이터 상한 = 1mib
		static constexpr size_t MAX_SEND_BUFFER = 1024 * 1024;

		static constexpr size_t MAX_RECV_BUFFER = Protocol::MAX_PACKET_SIZE * 2;

		static constexpr size_t MAX_PENDING_PACKETS = 4096;
	};

}