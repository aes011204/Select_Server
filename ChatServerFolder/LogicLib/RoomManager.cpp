#include "RoomManager.h"
#include <stdexcept>
#include <Windows.h>
#include <algorithm>
namespace NLogicLib
{
    NLogicLib::RoomManager::RoomManager(UserManager& users, RoomConfig config)
        : m_users(users), m_config (config)
    {
        //if (m_config.MaxRooms == 0 ||
        //    m_config.Capacity == 0 ||
        //    m_config.Capacity > 255)
        //{
        //    throw std::invalid_argument(
        //        "Invalid room configuration.");
        //}

        if (m_config.MaxRooms == 0 ||
            m_config.Capacity != Protocol::ROOM_PLAYER_COUNT)
        {
            throw std::invalid_argument(
                "Rooms must have exactly two player slots.");
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

        //  UTF-8·길이 정책
        if (!IsValidTitle(title))
        {
            return RoomResult::InvalidTitle;
        }

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
    RoomResult RoomManager::EnterRoom(NServerNetLib::SessionId sessionId, RoomId roomId)
    {
        User* user = m_users.FindMutable(sessionId);

        if (user == nullptr)
            return RoomResult::UserNotFound;

        if (user->GetState() != UserState::Lobby)
            return RoomResult::AlreadyInRoom;

        if (user->GetRoomId() != INVALID_ROOM_ID)
            return RoomResult::StateMismatch;

        const auto it = m_rooms.find(roomId);

        if (it == m_rooms.end())
            return RoomResult::RoomNotFound;

        Room& room = it->second;

        if (room.GetPhase() == Protocol::RoomPhase::Playing)
        {
            return RoomResult::GameInProgress;
        }

        if (room.Contains(sessionId))
            return RoomResult::StateMismatch;

        if (room.GetUserCount() >= room.GetCapacity())
            return RoomResult::RoomFull;

        if (!room.AddMember(sessionId))
            return RoomResult::StateMismatch;

        user->EnterRoom(roomId);


        return RoomResult::Success;
    }
    std::vector<Protocol::RoomInfo> RoomManager::GetRoomsAfter(RoomId afterRoomId, bool& outHasMore) const
    {
        std::vector<Protocol::RoomInfo> result;

        for (const auto& entry : m_rooms)
        {
            const Room& room = entry.second;

            if (room.GetId() <= afterRoomId)
            {
                continue;
            }

            Protocol::RoomInfo info;
            info.RoomId = room.GetId();
            info.UserCount = static_cast<UINT8>(room.GetUserCount());
            info.Capacity = static_cast<UINT8>(room.GetCapacity());
            info.Title = room.GetTitle();

            result.push_back(std::move(info));
        }

        std::sort(result.begin(), result.end(),
            [](const Protocol::RoomInfo& a,
                const Protocol::RoomInfo& b)
            {
                return a.RoomId < b.RoomId;
            });

        outHasMore =result.size() > Protocol::ROOM_LIST_PAGE_SIZE;

        if (outHasMore)
        {
            result.resize(Protocol::ROOM_LIST_PAGE_SIZE);
        }

        return result;
    }
    RoomResult RoomManager::SetReady(NServerNetLib::SessionId sessionId, bool ready)
    {
        const User* user = m_users.Find(sessionId);

        if (user == nullptr)
            return RoomResult::UserNotFound;

        if (user->GetState() != UserState::InRoom)
            return RoomResult::NotInRoom;

        const auto it = m_rooms.find(user->GetRoomId());

        if (it == m_rooms.end() ||!it->second.Contains(sessionId))
            return RoomResult::StateMismatch;
        
        Room& room = it->second;

        if (room.GetPhase() == Protocol::RoomPhase::Playing)
            return RoomResult::GameInProgress;

        if (room.GetHostSession() == sessionId)
            return RoomResult::HostCannotReady;

        room.SetReady(sessionId, ready);
        return RoomResult::Success;
    }

    RoomResult RoomManager::StartGame(NServerNetLib::SessionId sessionId)
    {
        const User* user = m_users.Find(sessionId);

        if (user == nullptr)
            return RoomResult::UserNotFound;

        if (user->GetState() != UserState::InRoom)
            return RoomResult::NotInRoom;

        const auto it = m_rooms.find(user->GetRoomId());

        if (it == m_rooms.end() ||!it->second.Contains(sessionId))
            return RoomResult::StateMismatch;

        Room& room = it->second;

        if (room.GetHostSession() != sessionId)
            return RoomResult::NotHost;

        if (room.GetPhase() == Protocol::RoomPhase::Playing)
            return RoomResult::GameAlreadyStarted;

        if (room.GetUserCount() != Protocol::ROOM_PLAYER_COUNT)
            return RoomResult::NotEnoughPlayers;

        for (auto member : room.GetMembers())
        {
            if (member == room.GetHostSession())
            {
                continue;
            }

            if (!room.IsReady(member))
                return RoomResult::NotAllReady;
        }

        room.StartGame();
        return RoomResult::Success;
    }

    bool RoomManager::IsValidTitle(const std::string& title) const
    {
        if (title.empty() ||title.size() > Protocol::MAX_ROOM_TITLE_BYTES)
            return false;

        bool hasNonSpace = false;

        for (unsigned char ch : title)
        {
            if (ch < 0x20 || ch == 0x7F)
                return false;

            if (ch != ' ')
            {
                hasNonSpace = true;
            }
        }

        if (!hasNonSpace)
            return false;
        
        //입력이 올바른 UTF-8인지 검사
        return MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            title.data(),
            static_cast<int>(title.size()),
            nullptr,
            0) > 0;
    }
}