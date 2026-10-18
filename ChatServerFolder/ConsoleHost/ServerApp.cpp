#include "ServerApp.h"
#include <iostream>
#include <conio.h>
NServer::ServerApp::ServerApp()
{
}

NServer::ServerApp::~ServerApp()
{
	Shutdown();
}

bool NServer::ServerApp::Init(const ServerConfig& config)
{
	if (m_initialized == true)
	{
		std::cerr << "Server is already initialized.\n";
		return false;
	}

	if (config.Network.Port == 0 ||
		config.LoginTimeout.count() <= 0)
	{
		std::cerr << "Invalid server configuration.\n";
		return false;
	}

	m_config = config;

	const auto result = m_network.Init(m_config.Network);

	if (result != NServerNetLib::NET_ERROR_CODE::NONE)
	{
		std::cerr<< "Network initialization failed. Code: "<< static_cast<int>(result)<< '\n';
		m_network.Release();
		return false;
	}

	m_logic = std::make_unique<NLogicLib::PacketProcessor>(m_network,m_config.LoginTimeout);

	m_initialized = true;

	std::cout<< "Listening on 127.0.0.1:"<< m_config.Network.Port<< '\n';

	return true;

}

int NServer::ServerApp::Run()
{

	if (m_initialized != true || !m_logic)
	{
		std::cerr << "Call Init() before Run().\n";
		return 1;
	}

	m_running = true;
	int exitCode = 0;

	std::cout << "Press Q to stop.\n";

	while(m_running)
	{
		if (_kbhit())
		{
			const int key = _getch();

			if (key == 'q' || key == 'Q')
			{
				Stop();
			}
		}

		if (!m_running)
		{
			break;
		}

		// 접속 수신 송신 패킷 조립을 한번 수행한다.
		if (m_network.Run()==false)
		{
			std::cerr << "Network processing failed. \n";

			exitCode = 1;
			Stop();
			break;
		}

		// 네트워크가 생성한 이벤트를 해석하고 처리한다.
		m_logic->Update();

	}

	m_running = false;
	return exitCode;
}

void NServer::ServerApp::Stop()
{
	m_running = false;
}

void NServer::ServerApp::Shutdown()
{
	Stop();

	// 네트워크를 참조하는 로직부터 제거한다
	m_logic.reset();

	// 소켓과  winsock 자원 정리
	m_network.Release();

	m_initialized = false;
}
