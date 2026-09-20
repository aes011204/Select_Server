#include "TcpNetwork.h"
#include <iostream>

namespace NServerNetLib
{

    TcpNetwork::TcpNetwork()
    {
    }

    TcpNetwork::~TcpNetwork()
    {
        Release();
    }

    NET_ERROR_CODE TcpNetwork::Init(UINT16 port)
    {
        // 이미 초기화된 상태에서 다시 호출되어도 먼저 정리
        Release();

        //1. winsock 초기화
        WSADATA wasData{};

        const int startupResult = WSAStartup(MAKEWORD(2, 2), &wasData);


        if (startupResult != 0)
        {
            std::cerr << "WSAStartup failed: " << startupResult << "\n";
            return NET_ERROR_CODE::SERVER_SOCKET_CREATE_FAIL;;
        }

        m_winsockStarted = true;

        // 2. IPv4 TCP 소켓생성
        m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//(IPv4 주소 사용,바이트 스트림 방식,TCP 프로토콜 사용)

        if (m_listenSocket == INVALID_SOCKET)
        {
            std::cerr << "WSAStartup failed: " << WSAGetLastError() << "\n";

            Release();

            return NET_ERROR_CODE::SERVER_SOCKET_CREATE_FAIL;
        }

        // + 논블로킹 설정 (생성 후 서버주소 구성 전)
        NET_ERROR_CODE err = SetNonBlockSocket(m_listenSocket);
        if (err != NET_ERROR_CODE::NONE)
        {
            return err;
        }

        // 3. 서버 주소 구성 
        sockaddr_in serverAddress{};
        serverAddress.sin_family = AF_INET; //IPv4 주소 사용
        serverAddress.sin_port = htons(port); // 16비트 값: 포트 번호
        serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK); //32비트 값: 여기서는 IPv4 주소(INADDR_LOOPBACK = 127.0.0.1 = 나 자신)

        //4. 소켓에 주소와 포트 연결 
        const int bindResult = bind(m_listenSocket, reinterpret_cast<const sockaddr*>(&serverAddress), sizeof(serverAddress));

        if (bindResult == SOCKET_ERROR)
        {
            std::cerr << "bind failed: " << WSAGetLastError()<< "\n";

            Release();
            return NET_ERROR_CODE::SERVER_SOCKET_BIND_FAIL;
        }

        //5. 접속 대기 상태로 전환
        //SOMAXCONN 운영체제의 소켓 제공자가 정하는 적절한 최대 대기열 크기를 사용하도록 요청하는 값
        if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR)
        {
            std::cerr << "listen failed: " << WSAGetLastError() << "\n";

            Release();
            return NET_ERROR_CODE::SERVER_SOCKET_LISTEN_FAIL;
        }


        return NET_ERROR_CODE::NONE;
    }

    bool TcpNetwork::Run()
    {
        //한 번 검사하고 반환하는 함수

        if (m_listenSocket == INVALID_SOCKET)
        {
            std::cerr << "Run called without a listening soket.\n";
            return false;
        }

        // 매번 검사할 소켓 집합을 새로 구성 
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(m_listenSocket, &readSet);

        // 처리할 일이 없으면 최대 100ms 대기 
        timeval timeout{};
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
        
        const int result = select(0, &readSet, nullptr, nullptr, &timeout);

        if(result ==SOCKET_ERROR)
        {
            std::cerr << "select failed: " << WSAGetLastError() << "\n";
            return false;
        }

        if (result == 0)
        {
            //타임 아웃 : 오류아님 처리할 일이 없었다는 뜻
            return true;
        }

        if (FD_ISSET(m_listenSocket, &readSet))
        {
            AcceptClient();
        }

        return true;
        // return true : 다음 반복을 계속해도 됨, 접속이 없었던 경우도 포함
    }

    void TcpNetwork::Release()
    {
        //1. 클라이언트 소켓 정리
        for (SOCKET clientSoket : m_clientSockets)
        {
            closesocket(clientSoket);
        }

        m_clientSockets.clear();

        //2. 리스닝 소켓정리 

        // 생성된 소켓이 있을 때만 닫기 
        if (m_listenSocket != INVALID_SOCKET)
        {
            closesocket(m_listenSocket);
            m_listenSocket = INVALID_SOCKET;
        }
        // 초기화에 성공했을 떄만 winsock 정리
        if (m_winsockStarted)
        {
            WSACleanup();
            m_winsockStarted = false;
        }

    }

    NET_ERROR_CODE TcpNetwork::AcceptClient()
    {
        SOCKET clientSocket = accept(m_listenSocket, nullptr, nullptr);//(리스닝소켓, 상대주소를받을공간, 주소공간의크기)

        if (clientSocket == INVALID_SOCKET)
        {
            const int errer = WSAGetLastError();

            // 논블로킹 소켓에 지금 받을 연결이 없는 경우
            if (errer == WSAEWOULDBLOCK)
            {
                return NET_ERROR_CODE::ACCEPT_API_WSAEWOULDBLOCK;
            }

            std::cerr << "accept failed:" << errer << "\n";
            return NET_ERROR_CODE::AACCEPT_API_ERROR;
        }
        
        NET_ERROR_CODE err = SetNonBlockSocket(clientSocket);
        if (err != NET_ERROR_CODE::NONE)
        {
            closesocket(clientSocket);
            return err;
        }

        m_clientSockets.push_back(clientSocket);

        std::cout << "Client accepted. Socket: " << clientSocket << ", stored sockets: " << m_clientSockets.size() << "\n";

    }

    NET_ERROR_CODE TcpNetwork::SetNonBlockSocket(const SOCKET sock)
    {

        u_long nonBlocking = 1; //FIONBIO는 블로킹 모드를 바꾸라는 명령, 값 1은 논블로킹을 의미

        if (ioctlsocket(sock, FIONBIO, &nonBlocking) == SOCKET_ERROR)
        {
            std::cerr << "ioctlsocket failed: " << WSAGetLastError << "\n";

            Release();
            return NET_ERROR_CODE::SERVER_SOCKET_FIONBIO_FAIL;
        }

        return NET_ERROR_CODE::NONE;
    }



}