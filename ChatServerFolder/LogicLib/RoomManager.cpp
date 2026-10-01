#include "RoomManager.h"
#include <stdexcept>
namespace NLogicLib
{
    NLogicLib::RoomManager::RoomManager(UserManager& users, RoomConfig config)
        : m_users(users), m_config (config)
    {
        if (m_config.MaxRooms == 0 ||
            m_config.Capacity == 0)
        {
            throw std::invalid_argument(
                "Invalid room configuration.");
        }
    }

    NLogicLib::RoomManager::~RoomManager()
    {
    }

    RoomResult NLogicLib::RoomManager::CreateRoom(NServerNetLib::SessionId sessionId, const std::string& title, RoomId& outRoomId)
    {
        outRoomId = INVALID_ROOM_ID;

        User* user = m_users.FindMutable(sessionId);

        if (user == nullptr)
            return RoomResult::UserNotFound;

        if (user->GetState() != UserState::Lobby)
            return RoomResult::AlreadyInRoom;

        if (user->GetRoomId() != INVALID_ROOM_ID)
            return RoomResult::StateMismatch;

        // 오늘은 내부 호출용으로 기본적인 빈 제목만 검사한다.
        // 외부 패킷을 연결할 때 UTF-8·길이 정책을 추가한다.
        if (title.empty())
            return RoomResult::InvalidTitle;

        if (m_rooms.size() >= m_config.MaxRooms)
            return RoomResult::RoomLimitReached;

        if (m_lastRoomId == (std::numeric_limits<RoomId>::max)())
            return RoomResult::RoomIdExhausted;

        const RoomId newRoomId = m_lastRoomId + 1;

        Room room(newRoomId,title, m_config.Capacity, sessionId);

        const auto inserted = m_rooms.emplace(newRoomId, std::move(room));

        if (!inserted.second) //삽입 성공 실패 판별
            return RoomResult::StateMismatch;

        // 방 등록이 성공한 뒤 사용자 상태를 변경한다.
        user->EnterRoom(newRoomId);

        m_lastRoomId = newRoomId;
        outRoomId = newRoomId;

        return RoomResult::Success;
    }

    RoomResult NLogicLib::RoomManager::LeaveRoom(NServerNetLib::SessionId sessionId)
    {
        User* user = m_users.FindMutable(sessionId);

        if (user == nullptr)
            return RoomResult::UserNotFound;

        if (user->GetState() == UserState::Lobby)
        {
            if (user->GetRoomId() != INVALID_ROOM_ID)
            {
                return RoomResult::StateMismatch;
            }
            return RoomResult::NotInRoom;
        }

        const RoomId roomId = user->GetRoomId();
        const auto roomIt = m_rooms.find(roomId);

        if (roomIt == m_rooms.end())
            return RoomResult::StateMismatch;

        Room& room = roomIt->second;

        if (!room.RemoveMember(sessionId))
            return RoomResult::StateMismatch;

        user->ReturnToLobby();

        if (room.IsEmpty())
            m_rooms.erase(roomIt);

        return RoomResult::Success;
    }

    const Room* NLogicLib::RoomManager::Find(RoomId roomId) const
    {
        const auto it = m_rooms.find(roomId);

        if (it == m_rooms.end())
            return nullptr;

        return &it->second;
    }
}