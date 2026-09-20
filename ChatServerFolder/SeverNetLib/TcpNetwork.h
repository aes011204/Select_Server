#pragma once

#include <winsock2.h>
#include <vector>
#include "ServerNetErrorCode.h"

namespace NServerNetLib
{

	struct ClientSession
	{
		//클라이언트마다소켓 번호뿐 아니라 버퍼 같은 정보도 함께 관리하기 위해
		SOCKET Socket = INVALID_SOCKET;
		std::vector<char> SendBuffer;
	};


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

		bool ReceiveClient(ClientSession& client);
		bool SendClient(ClientSession& client);
		void CloseClient(size_t index);

		NET_ERROR_CODE SetNonBlockSocket(const SOCKET sock);
	private:
		SOCKET m_listenSocket = INVALID_SOCKET;
		bool m_winsockStarted = false;

		std::vector<ClientSession> m_clients;

		// 읽기 집합에서 리스닝 소켓 한자리를 제외
		static constexpr size_t MAX_CLIENTS = FD_SETSIZE - 1;

		// 클라이언트 별 송산 대기 데이터 상한 = 1mib
		static constexpr size_t MAX_SEND_BUFFER = 1024 * 1024;
	};

}