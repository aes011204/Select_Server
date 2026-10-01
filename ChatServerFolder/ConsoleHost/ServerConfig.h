#pragma once

#include <Windows.h>
#include <chrono>

#include "../SeverNetLib/NetworkConfig.h"
namespace NServer
{
	struct ServerConfig
	{
		NServerNetLib::NetworkConfig Network;

		std::chrono::seconds LoginTimeout{ 10 };
	};
}
// 나중에 들어갈거 
//포트
//최대 접속자
//한 반복의 최대 접속 수락 수
//로그인 제한 시간
//최대 방 수