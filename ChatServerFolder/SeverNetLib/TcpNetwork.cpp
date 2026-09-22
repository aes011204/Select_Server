#include "TcpNetwork.h"
#include <iostream>
#include <limits>
#include <utility>
//#include <limits>
#include "../../Common/PacketUtils.h"
#include "../../Common/PacketID.h"


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
        //한 번 검사하고 반환하는 함수 / 루프는 호스트 쪽에서 

        if (m_listenSocket == INVALID_SOCKET)
        {
            std::cerr << "Run called without a listening soket.\n";
            return false;
        }

        // 매번 검사할 소켓 집합을 새로 구성 
        fd_set readSet;
        fd_set writeSet;
        FD_ZERO(&readSet); // 비우기
        FD_ZERO(&writeSet); // 비우기

        FD_SET(m_listenSocket, &readSet); //집합에 소켓 추가

        // 새 연결 감시 (연결된 클라들)
        bool hasPendingSend = false;

        for (const auto& client : m_clients)
        {
            //모든 클라이언트의 수신과 종료를 감시
            FD_SET(client.Socket, &readSet);

            if (!client.SendBuffer.empty())
            {
                FD_SET(client.Socket, &writeSet);
                hasPendingSend = true;
            }
        }


        // 처리할 일이 없으면 최대 100ms 대기 (밖에서 루프 계속 돌면 빡세니까)
        timeval timeout{};
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
        
        const int result = select(0, &readSet, hasPendingSend ? &writeSet : nullptr, nullptr, &timeout); //(Windows에서는 첫 번째 인자를 사용x,읽기 준비 상태를 검사할 소켓 집합,쓰기 준비 상태는 검사하지 않음,예외 상태는 검사하지 않음,처리할 일이 없을 때 기다릴 최대 시간)

        if(result == SOCKET_ERROR)
        {
            std::cerr << "select failed: " << WSAGetLastError() << "\n";
            return false;
        }

        if (result == 0)
        {
            //타임 아웃 : 오류아님 처리할 일이 없었다는 뜻
            return true;
        }

        // 기존 크라이언트부터 처리 
        size_t index = 0;

        while (index < m_clients.size())
        {
            auto& client = m_clients[index];
            bool keepConnection = true;

            if (FD_ISSET(client.Socket, &readSet))
            {
                //받아 송신 버퍼에 넣고
                keepConnection = ReceiveClient(client);
            }

            if (keepConnection && FD_ISSET(client.Socket, &writeSet))
            {
                //A에게 돌려보내는 것
                keepConnection = SendClient(client);
            }

            if (!keepConnection)
            {
                CloseClient(index);
                // 삭제로 다음 원소가 현재 위치로 이동했으므로 index를 증가시키지 않음
                continue;
            }
            ++index;
        }


        // 새로 들어오고 싶어하는 애들 accept, 이번에 들어온 연결은 다음 Run()부터 수신·송신 감시
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
        for (const auto& client : m_clients)
        {
            closesocket(client.Socket);
        }

        m_clients.clear();
        m_receivedPackets.clear();

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

    bool TcpNetwork::TryPopPacket(ReceivedPacket& outPacket)
    {

        if (m_receivedPackets.empty())
        {
            return false;
        }

        outPacket = std::move(m_receivedPackets.front());

        m_receivedPackets.pop_front();

        return true;
    }

    bool TcpNetwork::IsConnected(SessionId sessionId) const
    {
        for (const auto& client : m_clients)
        {
            if (client.Id == sessionId)
            {
                return true;
            }
        }

        return false;
    }

    bool TcpNetwork::SendPacket(SessionId sessionId, UINT16 packetId, const char* body, size_t bodySize)
    {
        for (auto& client : m_clients)
        {
            if (client.Id == sessionId)
            {
                return QueuePacket(client, packetId, body, bodySize);
            }
        }

        return false;
    }

    void TcpNetwork::Disconnect(SessionId sessionId)
    {
        for (size_t i = 0; i < m_clients.size(); ++i)
        {
            if (m_clients[i].Id == sessionId)
            {
                CloseClient(i);
                return;
            }
        }
    }

    NET_ERROR_CODE TcpNetwork::AcceptClient()
    {
        //들어오고 싶어 대기 중인 연결 하나를 꺼내서, 프로그램이 그 상대와 통신할 수 있는 소켓을 반환하는 함수

        SOCKET clientSocket = accept(m_listenSocket, nullptr, nullptr);//(리스닝소켓, 상대주소를받을공간, 주소공간의크기)

        if (clientSocket == INVALID_SOCKET)
        {
            const int errer = WSAGetLastError();

            // 논블로킹 소켓에 지금 받을 연결이 없는 경우
            if (errer == WSAEWOULDBLOCK)
            {
                return NET_ERROR_CODE::ACCEPT_API_WSAEWOULDBLOCK;
            }
            // 다른 오류
            std::cerr << "accept failed:" << errer << "\n";
            return NET_ERROR_CODE::AACCEPT_API_ERROR;
        }
        
        if (m_clients.size() >= MAX_CLIENTS)
        {
            std::cout<< "Client limit reached.\n";
            closesocket(clientSocket);
            return NET_ERROR_CODE::ACCEPT_MAX_SESSION_COUNT;
        }

        //ID가 넘쳐서 예전값을 재사용하지 않도록 확인
        if (m_lastSessionId == (std::numeric_limits<SessionId>::max)())
        {
            std::cerr << "Session ID exhausted.\n";
            closesocket(clientSocket);
            return NET_ERROR_CODE::SESSION_ID_EXHAUSTED;
        }

        NET_ERROR_CODE err = SetNonBlockSocket(clientSocket);
        if (err != NET_ERROR_CODE::NONE)
        {
            closesocket(clientSocket);
            return err;
        }

        ClientSession client;
        client.Id = ++m_lastSessionId;
        client.Socket = clientSocket;

        const SessionId sessionId = client.Id;


        m_clients.push_back(std::move(client));

        std::cout << "Client accepted. Session: " << sessionId
            << ", socket: " << clientSocket
            << ", clients: " << m_clients.size() << '\n';

        return NET_ERROR_CODE::NONE;
    }

    bool TcpNetwork::ReceiveClient(ClientSession& client)
    {
        //특정 클라이언트가 보낸 데이터를 서버(수신버퍼)에 쌓는 함수.

        char buffer[4096];

        const int received = recv(client.Socket, buffer, static_cast<int>(sizeof(buffer)), 0);//(데이터를 받을 연결,받은 바이트를 저장할 배열,이번에 받을 수 있는 최대 크기,특별한 옵션 없이 수신)

        if (received > 0)
        {
            //실제로 받은 데이터의 양 
            const auto receivedSize = static_cast<size_t>(received);

            if (client.RecvBuffer.size() + receivedSize > MAX_SEND_BUFFER)
            {
                std::cerr << "Send buffer limit exceeded. Socket: "
                    << client.Socket << '\n';
                return false;
            }

            // 에코 : 받은 바이트를 같은 클라이언트에게 돌려보냄
            // 일반적 : 데이터를 수신 버퍼에 모아서 패킷을 완성해야 함
            client.RecvBuffer.insert(client.RecvBuffer.end(), buffer, buffer + received);

            std::cout << "Received " << received << " bytes. Socket: " << client.Socket << '\n';

            return ProcessRecvBuffer(client);
        }

        if (received == 0)
        {
            std::cout << "Peer finished sending. Socket: " << client.Socket << '\n';

            return false;
        }

        const int error = WSAGetLastError();

        if (error == WSAEWOULDBLOCK)
        {
            return true;
        }

        std::cerr << "recv failed: " << error << ", socket: " << client.Socket << '\n';

        return false;
    }

    bool TcpNetwork::SendClient(ClientSession& client)
    {
        //준비된 데이터를 클라이언트에게 보냄

        if (client.SendBuffer.empty())
        {
            return true;
        }

        const int sent = send(client.Socket, client.SendBuffer.data(), static_cast<int>(client.SendBuffer.size()),0);

        if (sent > 0)
        {
            // 실제로 보낸 만큼만 제거
            client.SendBuffer.erase(client.SendBuffer.begin(), client.SendBuffer.begin() + sent);

            std::cout << "Sent " << sent << " bytes. Socket: " << client.Socket << "\n";

            return true;
        }

        if (sent == SOCKET_ERROR)
        {
            const int error = WSAGetLastError();

            if (error == WSAEWOULDBLOCK)
            {
                // 버퍼를 유지하고 다음 쓰기 가능시점에 재시도 

                return true;
            }

            std::cerr << "send failed: " << error << ", socket: " << client.Socket << "\n";
            
            return false;
        }

        std::cerr << "send mase no progress. Socket: " << client.Socket << "\n";

        return false;
    }

    bool TcpNetwork::ProcessRecvBuffer(ClientSession& client)
    {
        size_t readPos = 0;

        while (true)
        {
            const size_t available = client.RecvBuffer.size() - readPos;

            //1. 헤더조차 완성 되지 않았으면 다음 수신을 기다림
            if (available < Protocol::HEADER_SIZE)
            {
                break;
            }

            const char* packet = client.RecvBuffer.data() + readPos;

            const UINT16 totalSize = Protocol::ReadUInt16(packet);

            const UINT16 packetId = Protocol::ReadUInt16(packet + 2);

            //2. 길이검증
            if (totalSize < Protocol::HEADER_SIZE || totalSize > Protocol::MAX_PACKET_SIZE)
            {
                std::cerr << "Invalid packet size: " << totalSize << "\n";
                return false;
            }

            //3. 본문까지 모두 도착했는지 확인 
            if (available < totalSize)
            {
                break;
            }

            const size_t bodySize = totalSize - Protocol::HEADER_SIZE;

            const char* body = packet + Protocol::HEADER_SIZE;

            //4.완성된 패킷 하나 처리
            if (!EnqueueReceivedPacket(client, packetId, body, bodySize))
                return false;

            //5.다음 패킷의 시작위치로 이동 
            readPos += totalSize;
        }

        //처리한 부분만 제거하고 불완전한 부분은 보관
        if (readPos > 0)
        {
            client.RecvBuffer.erase(client.RecvBuffer.begin(), client.RecvBuffer.begin() + readPos);
        }


        return true;
    }

    //bool TcpNetwork::HandlePacket(ClientSession& client, UINT16 packetId, const char* body, size_t bodySize)
    //{

    //    std::cout << "Packet received. ID: " << packetId << ", body bytes: "<< bodySize << '\n';

    //    switch (packetId)
    //    {
    //    case Protocol::ECHO_REQ:
    //        return QueuePacket(client,Protocol::ECHO_RES,body,bodySize);
    //    default:
    //        std::cerr << "Unknown packet ID: " << packetId << "\n";
    //        return false;
    //    }



    //    return false;
    //}

    bool TcpNetwork::EnqueueReceivedPacket(ClientSession& client, UINT16 packetId, const char* body, size_t bodysize)
    {
        //완성된 수신 패킷을 처리 대기 큐에 넣는 함수

        if (m_receivedPackets.size() >= MAX_PENDING_PACKETS)
        {
            std::cerr << "Received packet queue is full.\n";
            return false;
        }

        ReceivedPacket packet;
        packet.Session = client.Id;
        packet.PacketId = packetId;

        if (bodysize > 0)
        {
            packet.Body.assign(body, body + bodysize);
        }

        m_receivedPackets.push_back(std::move(packet));

        return true;
    }

    bool TcpNetwork::QueuePacket(ClientSession& client, UINT16 packetId, const char* body, size_t bodySize)
    {
        // 패킷 크기와 본문 포이터 확인
        if (bodySize > 0 && body == nullptr)
        {
            return false;
        }
        const size_t totalSize = Protocol::HEADER_SIZE + bodySize;

        if (client.SendBuffer.size() + totalSize > MAX_SEND_BUFFER)
        {
            std::cerr << "Send buffer limit exceeded.\n";
            return false;
        }

        const size_t oldSize = client.SendBuffer.size();

        client.SendBuffer.resize(oldSize + totalSize);

        char* packet = client.SendBuffer.data() + oldSize;

        //헤더기록
        Protocol::WriteUInt16(packet, static_cast<UINT16>(totalSize));

        Protocol::WriteUInt16(packet + 2, packetId);

        //본문기록 '
        if (bodySize > 0)
        {
            memcpy(packet + Protocol::HEADER_SIZE, body, bodySize);
        }


        return true;
    }

    void TcpNetwork::CloseClient(size_t index)
    {
        //전달한 소켓 하나만 닫는 함수
        const SessionId sessionId = m_clients[index].Id;
        const SOCKET socket = m_clients[index].Socket;

        closesocket(socket);

        m_clients.erase(m_clients.begin() + index);

        std::cout << "Client removed. Socket: "
            << socket
            <<"Session : "
            << sessionId
            << ", sessions: "
            << m_clients.size()
            << '\n';

    }

    NET_ERROR_CODE TcpNetwork::SetNonBlockSocket(const SOCKET sock)
    {
        // ioctlsocket은 accept(), recv(), send()가 처리할 수 없을 때 기다리지 않고 반환하는 힘수

        u_long nonBlocking = 1; //FIONBIO는 블로킹 모드를 바꾸라는 명령, 값 1은 논블로킹을 의미

        if (ioctlsocket(sock, FIONBIO, &nonBlocking) == SOCKET_ERROR)
        {
            std::cerr << "ioctlsocket failed: " << WSAGetLastError() << "\n";

            //closesocket() 밖에서 할거임
            return NET_ERROR_CODE::SERVER_SOCKET_FIONBIO_FAIL;
        }

        return NET_ERROR_CODE::NONE;
    }



}