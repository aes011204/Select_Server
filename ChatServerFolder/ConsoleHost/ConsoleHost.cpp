#include "../SeverNetLib/TcpNetwork.h"

#include <iostream>

int main()
{
    TcpNetwork network;

    constexpr unsigned short port = 32452;

    if (!network.Init(port))
    {
        std::cerr << "server Init failed.\n";
        std::cout << "Press Enter to exit.\n";
        std::cin.get();
        return 1;
    }

    std::cout << "Listening on 127.0.0.1:"
        << port << '\n';

    std::cout << "Press Enter to stop.\n";
    std::cin.get();

    network.Release();

    std::cout << "Server stopped.\n";

    return 0;
}
