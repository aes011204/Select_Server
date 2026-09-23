// ChatClient.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "TcpClient.h"
#include <conio.h>
#include <iostream>
#include <iostream>
#include <string>
#include "../Common/PacketID.h"

using namespace NChatClient;

    int main()
    {
        TcpClient client;

        if (!client.Connect("127.0.0.1", 32452))
        {
            std::cerr << "Connection failed.\n";
            std::cout << "Press Enter to exit.\n";
            std::cin.get();

            return 1;
        }

        std::cout << "Connected to server.\n";
        std::cout << "1: Send Hello\n";
        std::cout << "2: Send two packets\n";
        std::cout << "3: Send 3000-byte body\n";
        std::cout << "Q: Quit\n";

        while (true)
        {
            if (_kbhit())
            {
                const int key = _getch();
                std::cout << "[KEY] " << key << '\n';
                if (key == 'q' || key == 'Q')
                {
                    break;
                }

                bool queued = true;

                switch (key)
                {
                case '1':
                    queued = client.SendPacket(Protocol::ECHO_REQ, "Hello sever");
                    break;

                case '2':
                    queued = client.SendPacket(Protocol::ECHO_REQ, "First");
                    if (queued)
                    {
                        queued = client.SendPacket(Protocol::ECHO_REQ, "Second");
                    }
                    break;

                case '3':
                    queued = client.SendPacket(Protocol::ECHO_REQ, std::string(3000,'A'));
                    break;

                default:
                    break;
                }

                if (!queued)
                {
                    std::cerr << "Could not queue request.\n";
                    break;
                }
            }
        
            if (!client.Run())
            {
                break;
            }

            ClientPacket packet;

            while (client.TryPopPacket(packet))
            {
                if (packet.PacketId != Protocol::ECHO_RES)
                {
                    std::cerr << "Unexpected packet ID: " << packet.PacketId << '\n';
                    continue;
                }

                const std::string text(packet.Body.begin(), packet.Body.end());

                std::cout << "[Echo] bytes: " << packet.Body.size() << ", text: ";

                if (text.size() <= 80)
                {
                    std::cout << text;
                }
                else
                {
                    std::cout << text.substr(0, 80) << "...";
                }

                std::cout << "\n";
            }
        
        }

        client.Disconnect();

        std::cout << "Client stopped.\n";

        return 0;
    }

    // 프로그램 실행: <Ctrl+F5> 또는 [디버그] > [디버깅하지 않고 시작] 메뉴
    // 프로그램 디버그: <F5> 키 또는 [디버그] > [디버깅 시작] 메뉴

    // 시작을 위한 팁: 
    //   1. [솔루션 탐색기] 창을 사용하여 파일을 추가/관리합니다.
    //   2. [팀 탐색기] 창을 사용하여 소스 제어에 연결합니다.
    //   3. [출력] 창을 사용하여 빌드 출력 및 기타 메시지를 확인합니다.
    //   4. [오류 목록] 창을 사용하여 오류를 봅니다.
    //   5. [프로젝트] > [새 항목 추가]로 이동하여 새 코드 파일을 만들거나, [프로젝트] > [기존 항목 추가]로 이동하여 기존 코드 파일을 프로젝트에 추가합니다.
    //   6. 나중에 이 프로젝트를 다시 열려면 [파일] > [열기] > [프로젝트]로 이동하고 .sln 파일을 선택합니다.
