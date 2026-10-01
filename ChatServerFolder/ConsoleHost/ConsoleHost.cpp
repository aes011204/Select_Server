#include "../SeverNetLib/TcpNetwork.h"
#include "../LogicLib/PacketProcessor.h"

#include "ServerApp.h"
#include <iostream>

int main()
{
    NServer::ServerConfig config;

    NServer::ServerApp app;
    if (!app.Init(config))
    {
        return 1;
    }

   

    const int exitCode = app.Run();

    app.Shutdown();

    std::cout << "Server stopped.\n";

    return exitCode;
}
