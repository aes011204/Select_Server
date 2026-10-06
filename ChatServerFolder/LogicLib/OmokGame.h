#pragma once

#include <array>
#include <windows.h>
#include "../../Common/GamePackets.h"


namespace NLogicLib
{
    using Stone = Protocol::GameStone;
    using GameStatus = Protocol::GameState;
   
    enum class MoveResult
    {
        Success,

        GameNotRunning,
        InvalidStone,
        OutOfBounds,
        NotYourTurn,
        Occupied
    };

    class OmokGame
    {
    public:
        OmokGame() = default;
        ~OmokGame() = default;

    public:
        static constexpr int BOARD_SIZE = 15;
        static constexpr int WIN_LENGTH = 5;

        void Reset();
        void Start();

        MoveResult TryPlaceStone(Stone cellState, int x, int y);

        GameStatus GetStatus() const { return m_status; };
        Stone GetNextTurn() const { return m_nextTurn; };
        size_t GetMoveCount() const { return m_moveCount; };

        Stone GetStone(int x, int y) const { return m_board[y][x]; };
        const std::array<std::array<Stone, BOARD_SIZE>, BOARD_SIZE>& GetBoard() const { return m_board; };

    private:
        bool IsInside(int x, int y) const { return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE; };
        int CountDirection(Stone cellState, int x, int y, int dx, int dy) const;
        bool HasWinningLine(Stone cellState, int x, int y) const;

    private:
        std::array<std::array<Stone, BOARD_SIZE>, BOARD_SIZE> m_board{};

        GameStatus m_status = GameStatus::NotStarted;
        Stone m_nextTurn = Stone::Empty;

        size_t m_moveCount = 0;
    };
}