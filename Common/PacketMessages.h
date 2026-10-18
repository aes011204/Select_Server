#pragma once

#include "PacketProtocol.h"
#include <string>
namespace Protocol
{
    struct LoginRequest
    {
        std::string Nickname;
    };

    struct LoginResponse
    {
        LoginResult Result = LoginResult::Success;
    };

    struct ChatRequest
    {
        std::string Message;
    };

    struct ChatResponse
    {
        ChatResult Result = ChatResult::Success;
    };

    struct ChatNotification
    {
        std::string Nickname;
        std::string Message;
    };
}