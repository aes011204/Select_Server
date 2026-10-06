#pragma once
#pragma once

#include "../Common/GamePackets.h"

#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>

struct ClientGameView
{
    using Board =
        std::array<
        std::array<
        Protocol::GameStone,
        Protocol::GAME_BOARD_SIZE>,
        Protocol::GAME_BOARD_SIZE>;

    std::uint32_t RoomId = 0;
    bool RoomPlaying = false;

    Protocol::GameState State =
        Protocol::GameState::NotStarted;

    Protocol::GameStone NextTurn =
        Protocol::GameStone::Empty;

    std::uint32_t MoveCount = 0;
    Board Cells{};

    void Reset(std::uint32_t roomId = 0)
    {
        RoomId = roomId;
        RoomPlaying = false;
        State = Protocol::GameState::NotStarted;
        NextTurn = Protocol::GameStone::Empty;
        MoveCount = 0;

        for (auto& row : Cells)
        {
            row.fill(Protocol::GameStone::Empty);
        }
    }

    void Begin(std::uint32_t roomId)
    {
        Reset(roomId);

        RoomPlaying = true;
        State = Protocol::GameState::Playing;
        NextTurn = Protocol::GameStone::Black;
    }

    bool Apply(const Protocol::GameMoveNotification& packet)
    {
        if (!RoomPlaying ||
            State != Protocol::GameState::Playing ||
            packet.RoomId != RoomId ||
            packet.X >= Protocol::GAME_BOARD_SIZE ||
            packet.Y >= Protocol::GAME_BOARD_SIZE ||
            packet.MoveCount != MoveCount + 1 ||
            packet.PlacedStone != NextTurn ||
            Cells[packet.Y][packet.X] !=
            Protocol::GameStone::Empty)
        {
            return false;
        }

        Cells[packet.Y][packet.X] = packet.PlacedStone;

        State = packet.State;
        NextTurn = packet.NextTurn;
        MoveCount = packet.MoveCount;

        return true;
    }

    void Print() const
    {
        std::cout << "\n    ";

        for (std::size_t x = 0;
            x < Protocol::GAME_BOARD_SIZE;
            ++x)
        {
            std::cout << std::setw(2) << x << ' ';
        }

        std::cout << '\n';

        for (std::size_t y = 0;
            y < Protocol::GAME_BOARD_SIZE;
            ++y)
        {
            std::cout << std::setw(2) << y << "  ";

            for (auto stone : Cells[y])
            {
                char symbol = '.';

                if (stone == Protocol::GameStone::Black)
                    symbol = 'X';
                else if (stone == Protocol::GameStone::White)
                    symbol = 'O';

                std::cout << ' ' << symbol << ' ';
            }

            std::cout << '\n';
        }

        std::cout << "X = Black, O = White\n";
        std::cout << "Moves: " << MoveCount << '\n';

        switch (State)
        {
        case Protocol::GameState::Playing:
            std::cout
                << "Next: "
                << (NextTurn == Protocol::GameStone::Black
                    ? "Black"
                    : "White")
                << '\n';
            break;

        case Protocol::GameState::BlackWon:
            std::cout << "Black wins.\n";
            break;

        case Protocol::GameState::WhiteWon:
            std::cout << "White wins.\n";
            break;

        case Protocol::GameState::Draw:
            std::cout << "Draw.\n";
            break;

        default:
            std::cout << "Game has not started.\n";
            break;
        }
    }
};