#include "Room.h"
#include <algorithm>
#include <stdexcept>
namespace NLogicLib
{
    NLogicLib::Room::Room(RoomId id, std::string title, size_t capacity, NServerNetLib::SessionId creator)
        : m_id(id), m_title(title),m_capacity(capacity), m_hostSession(creator)
    {
        if(id == INVALID_ROOM_ID || capacity ==0 || creator==0)
        {
            throw std::invalid_argument(
                "Invalid room construction.");
        }

        m_members.push_back(creator);
    }

    NLogicLib::Room::~Room()
    {
    }

    bool NLogicLib::Room::Contains(NServerNetLib::SessionId sessionId) const
    {
        return find( m_members.begin(), m_members.end(), sessionId) != m_members.end();
    }

    Stone Room::GetPlayerStone(NServerNetLib::SessionId sessionId) const
    {
        if (sessionId == 0)
            return Stone::Empty;

        if (sessionId == m_blackPlayer)
            return Stone::Black;

        if (sessionId == m_whitePlayer)
            return Stone::White;

        return Stone::Empty;
    }

    bool NLogicLib::Room::RemoveMember(NServerNetLib::SessionId sessionId)
    {
        const auto it = find(m_members.begin(), m_members.end(), sessionId);

        if (it == m_members.end())
            return false;

        m_members.erase(it);

        // 누가 나가면 게임을 중단하고 대기로 복귀.
        m_phase = Protocol::RoomPhase::Waiting;
        m_readyMembers.clear();

        m_game.Reset();
        m_blackPlayer = 0;
        m_whitePlayer = 0;

        if (m_members.empty())
        {
            m_hostSession = 0;
        }
        else if (m_hostSession == sessionId)
        {
            m_hostSession = m_members.front();
        }

        return true;
    }
    bool Room::AddMember(NServerNetLib::SessionId sessionId)
    {
        if (m_phase != Protocol::RoomPhase::Waiting || sessionId == 0 ||Contains(sessionId) || m_members.size() >= m_capacity)
        {
            return false;
        }

        m_members.push_back(sessionId);

        // 참가자 구성이 바뀌면 다시 준비
        m_readyMembers.clear();

        return true;
    }
    void Room::SetReady(NServerNetLib::SessionId sessionId, bool ready)
    {
        if (ready)
            m_readyMembers.insert(sessionId);
        else
            m_readyMembers.erase(sessionId);
    }
    void Room::StartGame()
    {
        m_blackPlayer = m_hostSession;
        m_whitePlayer = 0;

        for (auto member : m_members)
        {
            if (member != m_hostSession)
            {
                m_whitePlayer = member;
                break;
            }
        }

        m_game.Start();

        m_phase = Protocol::RoomPhase::Playing;

        // 준비는 이번 시작을 위한 상태였으므로 소비한다.
        m_readyMembers.clear();
    }
    MoveResult Room::PlaceStone(NServerNetLib::SessionId sessionId, int x, int y)
    {
        if (m_phase != Protocol::RoomPhase::Playing)
            return MoveResult::GameNotRunning;

        const Stone stone = GetPlayerStone(sessionId);

        if (stone == Stone::Empty)
            return MoveResult::InvalidStone;

        return m_game.TryPlaceStone(stone, x, y);
    }
}