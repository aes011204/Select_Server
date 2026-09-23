#pragma once

#include <winsock2.h>
#include <deque>
#include <string>
#include <vector>

#include "../Common/PacketProtocol.h"



namespace NChatClient
{
	struct ClientPacket
	{
		std::uint16_t PacketId = 0;
		std::vector<char> Body;
	};

	class TcpClient
	{
	public:
		TcpClient();
		~TcpClient();

		TcpClient(const TcpClient&) = delete;
		TcpClient& operator=(const TcpClient&) = delete;

	public:

		bool Connect(const char* ip, UINT16 port);
		bool Run();
		bool SendPacket(UINT16 PacketId, const std::string& body);
		bool TryPopPacket(ClientPacket& outPacket);
		void Disconnect();

	private:
		bool Receive();
		bool FlushSend();
		bool ProcessRecvBuffer();

	private:
		SOCKET m_socket = INVALID_SOCKET;
		bool m_winsockStarted = false;
		std::vector<char> m_recvBuffer;
		std::vector<char> m_sendBuffer;
		std::deque<ClientPacket> m_packets;

		static constexpr std::size_t MAX_RECV_BUFFER = Protocol::MAX_PACKET_SIZE * 2;

		static constexpr std::size_t MAX_SEND_BUFFER = 1024 * 1024;

		static constexpr std::size_t MAX_PENDING_PACKETS = 1024;
	};

}