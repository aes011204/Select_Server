#pragma once

#include <windows.h>

namespace Protocol
{
    constexpr std::size_t GAME_BOARD_SIZE = 15;


    //클라가 서버의 LogicLib에 의존하지 않도록 하기 위해 각각
    enum class GameStone : UINT8
    {
        Empty = 0,
        Black = 1,
        White = 2
    };

    enum class GameState : UINT8
    {
        NotStarted = 0,
        Playing = 1,
        BlackWon = 2,
        WhiteWon = 3,
        Draw = 4
    };

    enum class GameMoveResult : UINT8
    {
        Success = 0,

        NotLoggedIn = 1,
        NotInRoom = 2,
        WrongRoom = 3,

        GameNotRunning = 4,
        OutOfBounds = 5,
        NotYourTurn = 6,
        Occupied = 7,

        StateMismatch = 8
    };

    struct GameMoveRequest
    {
        UINT32 RoomId = 0;
        UINT8 X = 0;
        UINT8 Y = 0;
    };

    struct GameMoveResponse
    {
        GameMoveResult Result = GameMoveResult::Success;
    };

    struct GameMoveNotification
    {
        UINT32 RoomId = 0;

        UINT8 X = 0;
        UINT8 Y = 0;

        GameStone PlacedStone = GameStone::Empty;
        GameStone NextTurn = GameStone::Empty;

        GameState State = GameState::NotStarted;

        UINT32 MoveCount = 0;
    };
}