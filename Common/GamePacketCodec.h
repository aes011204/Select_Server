#pragma once

#include "GamePackets.h"
#include "RoomPacketCodec.h"

namespace Protocol
{
    inline bool Encode(const GameMoveRequest& packet,PacketBody& out)
    {
        RoomCodecDetail::Writer writer;

        writer.U32(packet.RoomId);
        writer.U8(packet.X);
        writer.U8(packet.Y);

        return writer.Finish(out);
    }

    inline bool Decode(const PacketBody& body,GameMoveRequest& out)
    {
        RoomCodecDetail::Reader reader(body);
        GameMoveRequest decoded;

        if (!reader.U32(decoded.RoomId) ||
            !reader.U8(decoded.X) ||
            !reader.U8(decoded.Y) ||
            !reader.Done())
        {
            return false;
        }

        out = decoded;
        return true;
    }

    inline bool Encode(const GameMoveResponse& packet,PacketBody& out)
    {
        RoomCodecDetail::Writer writer;
        writer.U8(static_cast<UINT8>(packet.Result));

        return writer.Finish(out);
    }

    inline bool Decode(const PacketBody& body,GameMoveResponse& out)
    {
        RoomCodecDetail::Reader reader(body);
        UINT8 result = 0;

        if (!reader.U8(result) ||
            result > static_cast<UINT8>(GameMoveResult::StateMismatch) ||
            !reader.Done())
        {
            return false;
        }

        out.Result = static_cast<GameMoveResult>(result);
        return true;
    }

    inline bool IsValidGameMove(
        const GameMoveNotification& packet)
    {
        if (packet.RoomId == 0 ||
            packet.X >= GAME_BOARD_SIZE ||
            packet.Y >= GAME_BOARD_SIZE ||
            packet.MoveCount == 0 ||
            packet.MoveCount >
            GAME_BOARD_SIZE * GAME_BOARD_SIZE)
        {
            return false;
        }

        if (packet.PlacedStone != GameStone::Black &&
            packet.PlacedStone != GameStone::White)
        {
            return false;
        }

        switch (packet.State)
        {
        case GameState::Playing:
            return packet.MoveCount <
                GAME_BOARD_SIZE * GAME_BOARD_SIZE &&
                packet.NextTurn ==
                (packet.PlacedStone == GameStone::Black
                    ? GameStone::White
                    : GameStone::Black);

        case GameState::BlackWon:
            return packet.PlacedStone == GameStone::Black &&
                packet.NextTurn == GameStone::Empty;

        case GameState::WhiteWon:
            return packet.PlacedStone == GameStone::White &&
                packet.NextTurn == GameStone::Empty;

        case GameState::Draw:
            return packet.NextTurn == GameStone::Empty &&
                packet.MoveCount ==
                GAME_BOARD_SIZE * GAME_BOARD_SIZE;

        default:
            return false;
        }
    }

    inline bool Encode(
        const GameMoveNotification& packet,
        PacketBody& out)
    {
        if (!IsValidGameMove(packet))
        {
            return false;
        }

        RoomCodecDetail::Writer writer;

        writer.U32(packet.RoomId);
        writer.U8(packet.X);
        writer.U8(packet.Y);

        writer.U8(
            static_cast<UINT8>(packet.PlacedStone));

        writer.U8(
            static_cast<UINT8>(packet.NextTurn));

        writer.U8(
            static_cast<UINT8>(packet.State));

        writer.U32(packet.MoveCount);

        return writer.Finish(out);
    }

    inline bool Decode(const PacketBody& body,GameMoveNotification& out)
    {
        RoomCodecDetail::Reader reader(body);
        GameMoveNotification decoded;

        UINT8 placed = 0;
        UINT8 next = 0;
        UINT8 state = 0;

        if (!reader.U32(decoded.RoomId) ||
            !reader.U8(decoded.X) ||
            !reader.U8(decoded.Y) ||
            !reader.U8(placed) ||
            !reader.U8(next) ||
            !reader.U8(state) ||
            !reader.U32(decoded.MoveCount) ||
            !reader.Done())
        {
            return false;
        }

        decoded.PlacedStone = static_cast<GameStone>(placed);
        decoded.NextTurn = static_cast<GameStone>(next);
        decoded.State = static_cast<GameState>(state);

        if (!IsValidGameMove(decoded))
        {
            return false;
        }

        out = decoded;
        return true;
    }

    //종료 패킷 코덱
    inline bool Encode(
        const GameResignRequest& packet,
        PacketBody& out)
    {
        RoomCodecDetail::Writer writer;
        writer.U32(packet.RoomId);
        return writer.Finish(out);
    }

    inline bool Decode(
        const PacketBody& body,
        GameResignRequest& out)
    {
        RoomCodecDetail::Reader reader(body);
        GameResignRequest decoded;

        if (!reader.U32(decoded.RoomId) || !reader.Done())
        {
            return false;
        }

        out = decoded;
        return true;
    }

    inline bool IsValidGameEnd(
        const GameEndNotification& packet)
    {
        if (packet.RoomId == 0 ||
            packet.MoveCount >
            GAME_BOARD_SIZE * GAME_BOARD_SIZE)
        {
            return false;
        }

        const bool hasWinner =
            packet.State == GameState::BlackWon ||
            packet.State == GameState::WhiteWon;

        switch (packet.Reason)
        {
        case GameEndReason::FiveInRow:
            return hasWinner;

        case GameEndReason::BoardFull:
            return packet.State == GameState::Draw &&
                packet.MoveCount ==
                GAME_BOARD_SIZE * GAME_BOARD_SIZE;

        case GameEndReason::Resigned:
        case GameEndReason::TurnTimeout:
            return hasWinner;

        default:
            return false;
        }
    }

    inline bool Encode(
        const GameEndNotification& packet,
        PacketBody& out)
    {
        if (!IsValidGameEnd(packet))
        {
            return false;
        }

        RoomCodecDetail::Writer writer;

        writer.U32(packet.RoomId);
        writer.U8(static_cast<std::uint8_t>(packet.State));
        writer.U8(static_cast<std::uint8_t>(packet.Reason));
        writer.U32(packet.MoveCount);

        return writer.Finish(out);
    }

    inline bool Decode(
        const PacketBody& body,
        GameEndNotification& out)
    {
        RoomCodecDetail::Reader reader(body);
        GameEndNotification decoded;

        std::uint8_t state = 0;
        std::uint8_t reason = 0;

        if (!reader.U32(decoded.RoomId) ||
            !reader.U8(state) ||
            !reader.U8(reason) ||
            !reader.U32(decoded.MoveCount) ||
            !reader.Done())
        {
            return false;
        }

        decoded.State = static_cast<GameState>(state);
        decoded.Reason = static_cast<GameEndReason>(reason);

        if (!IsValidGameEnd(decoded))
        {
            return false;
        }

        out = decoded;
        return true;
    }
}