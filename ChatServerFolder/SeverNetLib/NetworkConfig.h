#pragma once

#include <chrono>
#include <windows.h>
#include <cstddef>
#include <cstdint>

namespace NServerNetLib
{
    struct NetworkConfig
    {
        UINT16 Port = 32452;

        size_t MaxClients = 63; //동시에 유지할 연결의 최대 개수

        size_t MaxAcceptsPerRun = 16; //네트워크 반복 한 번에서 시도할 최대 accept 횟수

        size_t MaxRecvBufferBytes = 8192;

        size_t MaxSendBufferBytes = 1024 * 1024;

        // Packet뿐 아니라 Connected/Disconnected까지 포함
        size_t MaxQueuedEvents = 8192;

        std::chrono::milliseconds SelectTimeout{ 10 };
    };
}

// 예전 설정
//// 읽기 집합에서 리스닝 소켓 한자리를 제외
//static constexpr size_t MAX_CLIENTS = FD_SETSIZE - 1;
//
//// 클라이언트 별 송산 대기 데이터 상한 = 1mib
//static constexpr size_t MAX_SEND_BUFFER = 1024 * 1024;
//
//static constexpr size_t MAX_RECV_BUFFER = Protocol::MAX_PACKET_SIZE * 2;
//
//static constexpr size_t MAX_PENDING_PACKETS = 4096;