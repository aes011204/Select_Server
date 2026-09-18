#include "TcpNetwork.h"
#include <iostream>



TcpNetwork::TcpNetwork()
{
}

TcpNetwork::~TcpNetwork()
{
    Release();
}

bool TcpNetwork::Init(UINT16 port)
{
    // 이미 초기화된 상태에서 다시 호출되어도 먼저 정리
    Release();

    //1. winsock 초기화
    WSADATA wasData{};

    const int startupResult = WSAStartup(MAKEWORD(2,2), &wasData);


    if (startupResult != 0)
    {
        std::cerr << "WSAStartup failed: " << startupResult << "\n";
        return false;
    }

    m_winsockStarted = true;

    // 2. IPv4 TCP 소켓생성
    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//(IPv4 주소 사용,바이트 스트림 방식,TCP 프로토콜 사용)

    if (m_listenSocket == INVALID_SOCKET)
    {
        std::cerr << "WSAStartup failed: " << startupResult << "\n";
        
        Release();

        return false;
    }

    // 3. 서버 주소 구성 
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET; //IPv4 주소 사용
    serverAddress.sin_port = htons(port); // 16비트 값: 포트 번호
    serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK); //32비트 값: 여기서는 IPv4 주소(INADDR_LOOPBACK = 127.0.0.1 = 나 자신)

    //4. 소켓에 주소와 포트 연결 
    const int bindResult = bind(m_listenSocket,reinterpret_cast<const sockaddr*>(&serverAddress), sizeof(serverAddress));

    if (bindResult == SOCKET_ERROR)
    {
        std::cerr << "bind failed: " << startupResult << "\n";

        Release();
        return false;
    }

    //5. 접속 대기 상태로 전환
    //SOMAXCONN 운영체제의 소켓 제공자가 정하는 적절한 최대 대기열 크기를 사용하도록 요청하는 값
    if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cerr << "listen failed: " << startupResult << "\n";

        Release();
        return false;
    }
   

    return true;
}

void TcpNetwork::Release()
{
    // 생성된 소켓이 있을 때만 닫기 
    if (m_listenSocket != INVALID_SOCKET)
    {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }
    // 초기화에 성공했을 떄만 winsock 정리
    if(m_winsockStarted)
    {
        WSACleanup();
        m_winsockStarted = false;
    }

}
