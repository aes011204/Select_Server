#pragma once

#include <winsock2.h>
#include <vector>
#include "ServerNetErrorCode.h"

namespace NServerNetLib
{

	class TcpNetwork
	{
	public:
		TcpNetwork();
		~TcpNetwork();

		TcpNetwork(const TcpNetwork&) = delete;
		TcpNetwork& operator= (const TcpNetwork&) = delete;


		NET_ERROR_CODE Init(UINT16 port);
		bool Run();
		void Release();
	private:
		NET_ERROR_CODE AcceptClient();

		NET_ERROR_CODE SetNonBlockSocket(const SOCKET sock);
	private:
		SOCKET m_listenSocket = INVALID_SOCKET;
		bool m_winsockStarted = false;

		std::vector<SOCKET> m_clientSockets;
	};

}