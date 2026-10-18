#pragma once

#include "../SeverNetLib/TcpNetwork.h"
#include "../LogicLib/PacketProcessor.h"
#include "ServerConfig.h"
#include <memory>
namespace NServer
{
	class ServerApp
	{

	public:
		ServerApp();
		virtual ~ServerApp();

		ServerApp(const ServerApp&) = delete;
		ServerApp& operator=(const ServerApp&) = delete;

	public:
		bool Init(const ServerConfig& config);
		int Run();

		void Stop();
		void Shutdown();

	private:
		ServerConfig m_config;

		// 로직보다 먼저 생성되고 나중에 파괴되도록 선언한다
		NServerNetLib::TcpNetwork m_network;

		std::unique_ptr<NLogicLib::PacketProcessor> m_logic;

		bool m_initialized = false;
		bool m_running = false;

	};

}

