#include "OmokGame.h"

namespace NLogicLib
{
    void NLogicLib::OmokGame::Reset()
    {
        for (auto& row : m_board)
        {
            row.fill(Stone::Empty);
        }

        m_status = GameStatus::NotStarted;
        m_nextTurn = Stone::Empty;
        m_moveCount = 0;
    }

    void NLogicLib::OmokGame::Start()
    {
        Reset();

        m_status = GameStatus::Playing;
        m_nextTurn = Stone::Black;
    }

    MoveResult NLogicLib::OmokGame::TryPlaceStone(Stone cellState, int x, int y)
    {
        if (m_status != GameStatus::Playing)
            return MoveResult::GameNotRunning;

        if (cellState != Stone::Black && cellState != Stone::White)
            return MoveResult::InvalidStone;

        if (!IsInside(x, y))
            return MoveResult::OutOfBounds;

        if (cellState != m_nextTurn)
            return MoveResult::NotYourTurn;

        if (m_board[y][x] != Stone::Empty)
            return MoveResult::Occupied;

        // 모든 검사를 통과한 뒤 상태를 변경한다.
        m_board[y][x] = cellState;
        ++m_moveCount;

        if (HasWinningLine(cellState, x, y))
        {
            m_status = cellState == Stone::Black ? GameStatus::BlackWon : GameStatus::WhiteWon;

            m_nextTurn = Stone::Empty;
            return MoveResult::Success;
        }

        const auto boardCellCount = static_cast<size_t>(BOARD_SIZE) * BOARD_SIZE;

        if (m_moveCount == boardCellCount)
        {
            m_status = GameStatus::Draw;
            m_nextTurn = Stone::Empty;
            return MoveResult::Success;
        }

        m_nextTurn = cellState == Stone::Black ? Stone::White : Stone::Black;

        return MoveResult::Success;
    }

    int NLogicLib::OmokGame::CountDirection(Stone stone, int x, int y, int dx, int dy) const
    {
        int count = 0;

        int nextX = x + dx;
        int nextY = y + dy;

        while (IsInside(nextX, nextY))
        {
            if (m_board[nextY][nextX] != stone)
            {
                break;
            }

            ++count;

            nextX += dx;
            nextY += dy;
        }

        return count;

    }

    bool NLogicLib::OmokGame::HasWinningLine(Stone cellState, int x, int y) const
    {
        int directions[4][2] = { { 1,  0 },{ 0,  1 },{ 1,  1 },{ 1, -1 } };

        for (const auto& direction : directions)
        {
            const int dx = direction[0];
            const int dy = direction[1];

            const int count = 1 +
                CountDirection(cellState, x, y, dx, dy) + CountDirection(cellState, x, y, -dx, -dy);

            if (count >= WIN_LENGTH)
                return true;
        }

        return false;
    }
}