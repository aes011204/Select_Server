#include "TcpClient.h"
#include <ws2tcpip.h>
#include <iostream>

#include "../Common/PacketUtils.h"


NChatClient::TcpClient::TcpClient()
{
}

NChatClient::TcpClient::~TcpClient()
{
	Disconnect();
}

bool NChatClient::TcpClient::Connect(const char* ip, UINT16 port)
{
	Disconnect();

	WSADATA wsaData{};

	const int startupResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (startupResult != 0)
	{
		std::cerr << "WSAStarResult failed: " << startupResult << "\n";
		return false;
	}

	m_winsockStarted = true;

	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (m_socket == INVALID_SOCKET)
	{
		std::cerr << "socket failed: " << WSAGetLastError << "\n";
		Disconnect();
		return false;
	}

	sockaddr_in severAddress{};
	severAddress.sin_family = AF_INET;
	severAddress.sin_port = htons(port);

	const int addressResult = InetPtonA(AF_INET, ip, &severAddress.sin_addr);

	if (addressResult != 1)
	{
		std::cerr << "Invalid IPv4 address or conversion error.\n";
		Disconnect();
		return false;
	}

	// 임시) 연결단계만 블로킹 방식사용 나중에 ->블로킹 소켓 + 별도 송수신 스레드’
	std::cout << "[CONNECT] " << ip << ":" << port << '\n';
	if (connect(m_socket, reinterpret_cast<sockaddr*>(&severAddress),sizeof(severAddress)) == SOCKET_ERROR)
	{
		std::cerr << "connect failed: "
			<< WSAGetLastError() << '\n';

		Disconnect();
		return false;
	}

	// 연결 성공 후 송수신은 논블로킹으로 처리
	u_long nonBlocking = 1;
	if (ioctlsocket(m_socket, FIONBIO, &nonBlocking) == SOCKET_ERROR)
	{
		std::cerr << "ioctlsock failed: " << WSAGetLastError() << "\n";
		Disconnect();
		return false;
	}


	return true;
}

bool NChatClient::TcpClient::Run()
{

	if (m_socket == INVALID_SOCKET)
	{
		return false;
	}

	fd_set readSet;
	fd_set writeSet;

	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);

	FD_SET(m_socket, &readSet);

	const bool hasPendingSend = !m_sendBuffer.empty();

	if (hasPendingSend)
	{
		FD_SET(m_socket, &writeSet);
	}

	timeval timeout{};
	timeout.tv_sec = 0;
	timeout.tv_usec = 100000;

	const int result = select(0, &readSet, hasPendingSend ? &writeSet : nullptr, nullptr, &timeout);

	if (result == SOCKET_ERROR)
	{
		std::cerr << "select failed: " << WSAGetLastError() << '\n';

		return false;
	}

	if (result == 0)
	{
		return true;
	}


	if (FD_ISSET(m_socket, &readSet))
	{
		if (!Receive())
		{
			return false;
		}
	}

	if (FD_ISSET(m_socket, &writeSet))
	{
		if (!FlushSend())
		{
			return false;
		}
	}




	return true;
}

bool NChatClient::TcpClient::SendPacket(UINT16 packetId, const std::string& body)
{

	if (m_socket == INVALID_SOCKET)
	{
		return false;
	}

	if (body.size() > Protocol::MAX_PACKET_SIZE - Protocol::HEADER_SIZE)
	{
		std::cerr << "Packet body is too large.\n";
		return false;
	}

	const size_t  totalSize = Protocol::HEADER_SIZE + body.size();

	if (m_sendBuffer.size() + totalSize > MAX_SEND_BUFFER)
	{
		std::cerr << "Send buffer limit exceeded.\n";
		return false;
	}
	
	const size_t oldSize = m_sendBuffer.size();

	m_sendBuffer.resize(oldSize + totalSize);

	char* packet = m_sendBuffer.data() + oldSize;
	
	Protocol::WriteUInt16(packet, static_cast<UINT16>(totalSize));
	Protocol::WriteUInt16(packet+2, packetId);

	if (!body.empty())
	{
		std::memcpy(packet + Protocol::HEADER_SIZE, body.data(), body.size());
	}
	return true;

}

bool NChatClient::TcpClient::TryPopPacket(ClientPacket& outPacket)
{
	if (m_packets.empty())
	{
		return false;
	}

	outPacket = std::move(m_packets.front());
	m_packets.pop_front();

	return true;
}

void NChatClient::TcpClient::Disconnect()
{
	if (m_socket != INVALID_SOCKET)
	{
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}

	m_recvBuffer.clear();
	m_sendBuffer.clear();
	m_packets.clear();

	if (m_winsockStarted)
	{
		WSACleanup();
		m_winsockStarted = false;
	}

}

bool NChatClient::TcpClient::Receive()
{

	char buffer[4096];

	const int received = recv(m_socket, buffer, static_cast<int>(sizeof(buffer)), 0);//m_socket이 가리키는 연결로 도착한 데이터를 buffer에 넣어 줘.

	if (received > 0)
	{
		const auto receivedSize = static_cast<size_t>(received);

		if (m_recvBuffer.size() + receivedSize > MAX_RECV_BUFFER)
		{
			std::cerr << "Receive buffer limit exceeded.\n";
			return false;
		}

		m_recvBuffer.insert(m_recvBuffer.end(), buffer, buffer + received);

		return ProcessRecvBuffer();
	}

	if (received == 0)
	{
		if (!m_recvBuffer.empty())
		{
			std::cerr << "Incomplete packet at disconnect.\n";
		}

		std::cout << "Server closed the connection.\n";
		return false;
	}

	const int error = WSAGetLastError();

	if (error == WSAEWOULDBLOCK)
	{
		return true;
	}

	std::cerr << "recv failed: "
		<< error << '\n';

	return false;

}

bool NChatClient::TcpClient::FlushSend()
{
	if (m_sendBuffer.empty())
	{
		return true;
	}

	const int sent = send(m_socket, m_sendBuffer.data(), static_cast<int>(m_sendBuffer.size()), 0);

	if (sent > 0)
	{
		m_sendBuffer.erase(m_sendBuffer.begin(), m_sendBuffer.begin() + sent);

		return true;
	}

	if (sent == SOCKET_ERROR)
	{
		const int error = WSAGetLastError();

		if (error == WSAEWOULDBLOCK)
		{
			return true;// 연결 유지, 남은 데이터는 나중에 재시도
		}


		std::cerr << "send failed: "<< error << '\n';

		return false;
	}

	std::cerr << "send made no progress.\n";
	return false;
}

bool NChatClient::TcpClient::ProcessRecvBuffer()
{
	size_t readPos = 0;

	while (true)
	{
		const size_t available = m_recvBuffer.size() - readPos;

		if (available < Protocol::HEADER_SIZE)
		{
			break;
		}

		const char* packet = m_recvBuffer.data() + readPos;
		const UINT16 totalSize = Protocol::ReadUInt16(packet);
		const UINT16 packetId = Protocol::ReadUInt16(packet+2);

		if (totalSize < Protocol::HEADER_SIZE || totalSize > Protocol::MAX_PACKET_SIZE)
		{
			std::cerr << "Invalid packet size: " << totalSize << '\n';

			return false;
		}

		if (available < totalSize)
		{
			break;
		}
	
		if (m_packets.size() >= MAX_PENDING_PACKETS)
		{
			std::cerr << "packet queue is full \n";
			return false;
		}

		const size_t bodySize = totalSize - Protocol::HEADER_SIZE;
		const char* body = packet + Protocol::HEADER_SIZE;

		ClientPacket receivedPacket;
		receivedPacket.PacketId = packetId;

		if (bodySize > 0)
		{
			receivedPacket.Body.assign(body, body + bodySize);
		}

		m_packets.push_back(std::move(receivedPacket));

		readPos += totalSize;


	}

	if (readPos > 0)
	{
		m_recvBuffer.erase(m_recvBuffer.begin(), m_recvBuffer.begin() + static_cast<std::ptrdiff_t>(readPos));
	}



	return true;
}
