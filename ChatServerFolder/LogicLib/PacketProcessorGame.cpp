#include "PacketProcessor.h"

#include "../../Common/PacketID.h"
#include "../../Common/GamePacketCodec.h"

#include <iostream>

namespace NLogicLib
{
    Protocol::GameStone ToPacketStone(NLogicLib::Stone stone)
    {
        switch (stone)
        {
        case NLogicLib::Stone::Empty:
            return Protocol::GameStone::Empty;

        case NLogicLib::Stone::Black:
            return Protocol::GameStone::Black;

        case NLogicLib::Stone::White:
            return Protocol::GameStone::White;
        }

        throw std::logic_error("Unknown internal stone");
    }

    Protocol::GameState ToPacketState( NLogicLib::GameStatus status)
    {
        switch (status)
        {
        case NLogicLib::GameStatus::NotStarted:
            return Protocol::GameState::NotStarted;

        case NLogicLib::GameStatus::Playing:
            return Protocol::GameState::Playing;

        case NLogicLib::GameStatus::BlackWon:
            return Protocol::GameState::BlackWon;

        case NLogicLib::GameStatus::WhiteWon:
            return Protocol::GameState::WhiteWon;

        case NLogicLib::GameStatus::Draw:
            return Protocol::GameState::Draw;
        }

        throw std::logic_error("Unknown internal game status");
    }

    bool PacketProcessor::SendGameMoveResult(SessionId sessionId,Protocol::GameMoveResult result)
    {
        Protocol::GameMoveResponse response;
        response.Result = result;

        Protocol::PacketBody body;

        if (!Protocol::Encode(response, body) ||
            !m_network.SendPacket(
                sessionId,
                Protocol::GAME_MOVE_RES,
                body.data(),
                body.size()))
        {
            m_network.Disconnect(sessionId);
            return false;
        }

        return true;
    }

    void PacketProcessor::HandleGameMove(const NServerNetLib::NetworkEvent& event)
    {
        Protocol::GameMoveRequest request;

        if (!Protocol::Decode(event.Body, request))
        {
            m_network.Disconnect(event.Session);
            return;
        }

        AcceptedMove accepted;

        const auto result = m_rooms.PlaceStone(event.Session, request.RoomId, request.X, request.Y, accepted);

        if (result != Protocol::GameMoveResult::Success)
        {
            SendGameMoveResult(event.Session, result);
            return;
        }

        const Room* room = m_rooms.Find(accepted.Room);

        if (room == nullptr)
        {
            std::cerr << "Room disappeared after accepted move.\n";
            m_network.Disconnect(event.Session);
            return;
        }

        const auto targets = room->GetMembers();

        Protocol::GameMoveNotification notification;

        notification.RoomId = accepted.Room;
        notification.X = static_cast<UINT8>(accepted.X);
        notification.Y = static_cast<UINT8>(accepted.Y);

        notification.PlacedStone = ToPacketStone(accepted.PlacedStone);
        notification.NextTurn = ToPacketStone(accepted.NextTurn);
        notification.State = ToPacketState(accepted.Status);

        notification.MoveCount = static_cast<UINT32>(accepted.MoveCount);

        Protocol::PacketBody body;

        if (!Protocol::Encode(notification, body))
        {
            std::cerr << "Could not encode accepted move.\n";

            // 변경된 보드를 알릴 수 없으므로 참가 연결을 종료한다.
            // 종료 이벤트가 방과 게임을 정리한다.
            for (SessionId target : targets)
            {
                m_network.Disconnect(target);
            }

            return;
        }

        // 착수는 이미 서버 보드에 반영되었다.
        SendGameMoveResult(event.Session,Protocol::GameMoveResult::Success);

        for (SessionId target : targets)
        {
            if (!m_network.IsConnected(target))
            {
                continue;
            }

            if (!m_network.SendPacket(
                target,
                Protocol::GAME_MOVE_NTF,
                body.data(),
                body.size()))
            {
                m_network.Disconnect(target);
            }
        }
    }
}