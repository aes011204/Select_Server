#pragma once

#include <windows.h>
#include "../../Common/RoomPackets.h"
namespace NLogicLib
{
    using RoomId = UINT32;
    using RoomResult = Protocol::RoomResult;

    constexpr RoomId INVALID_ROOM_ID = 0;
    enum class UserState
    {
        Lobby,
        InRoom
    };

    struct RoomConfig
    {
        size_t MaxRooms = 100;
        size_t Capacity = 2;
    };

    /*enum class RoomResult
    {
        Success,

        UserNotFound,
        AlreadyInRoom,
        NotInRoom,

        InvalidTitle,
        RoomLimitReached,
        RoomIdExhausted,

        StateMismatch
    };*/
    // common 으로 
}