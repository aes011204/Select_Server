#include "../SeverNetLib/TcpNetwork.h"
#include "../LogicLib/PacketProcessor.h"
#include <conio.h>
#include <iostream>

int main()
{
    NServerNetLib::TcpNetwork network;

    constexpr unsigned short port = 32452;

    if (network.Init(port)!= NServerNetLib::NET_ERROR_CODE::NONE)
    {
        std::cerr << "server Init failed.\n";
        std::cout << "Press Enter to exit.\n";
        std::cin.get();
        return 1;
    }

    NLogicLib::PacketProcessor logic(network);

    std::cout << "Listening on 127.0.0.1:"
        << port << '\n';
    std::cout << "Press Q to stop.\n";

    int exitCode = 0;

    while (true)
    {
        // 키입력이 있을때만 읽기 
        if (_kbhit())
        {
            const int key = _getch();

            if (key == 'q' || key == 'Q')
            {
                break;
            }
        }
        //접속·수신·송신·패킷 조립
        if (!network.Run())
        {
            std::cerr << "Network loop stopped by an errer. \n";
            exitCode = 1;
            break;
        }
        // 조립된 패킷의 의미 처리
        logic.Update();
    }

    network.Release();

    std::cout << "Server stopped.\n";

    return exitCode;
}
