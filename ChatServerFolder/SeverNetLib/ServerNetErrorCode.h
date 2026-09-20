#pragma once

namespace NServerNetLib
{
    enum class NET_ERROR_CODE
    {
        NONE = 0,                   // 성공
        WINSOCK_INIT_FAIL,           // Winsock 초기화 실패
        SERVER_SOCKET_CREATE_FAIL,  // 소켓 생성 실패
        SERVER_SOCKET_BIND_FAIL,    // 주소·포트 설정 실패
        SERVER_SOCKET_LISTEN_FAIL,   // 접속 대기 설정 실패
        SERVER_SOCKET_FIONBIO_FAIL,  //

        ACCEPT_API_WSAEWOULDBLOCK,
        AACCEPT_API_ERROR,
        ACCEPT_MAX_SESSION_COUNT
    };
}