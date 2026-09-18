#pragma once

#include <winsock2.h>

class TcpNetwork
{
public:
	TcpNetwork();
	~TcpNetwork();

	TcpNetwork(const TcpNetwork&) = delete;
	TcpNetwork& operator= (const TcpNetwork&) = delete;


	bool Init(UINT16 port);
	void Release();

private:
	SOCKET m_listenSocket = INVALID_SOCKET;
	bool m_winsockStarted = false;
};

