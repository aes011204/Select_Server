#pragma once

#include <windows.h>
namespace NLogicLib
{
    using RoomId = UINT32;

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

    enum class RoomResult
    {
        Success,

        UserNotFound,
        AlreadyInRoom,
        NotInRoom,

        InvalidTitle,
        RoomLimitReached,
        RoomIdExhausted,

        StateMismatch
    };
}