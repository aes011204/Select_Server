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
            m_config.Capacity != Protocol::ROOM_PLAYER_COUNT ||
            m_config.TurnTime.count() <= 0)
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

        Room room(newRoomId,title, m_config.Capacity, sessionId, m_config.TurnTime);

        const auto inserted = m_rooms.emplace(newRoomId, std::move(room));

        if (!inserted.second) //삽입 성공 실패 판별
            return RoomResult::StateMismatch;

        // 방 등록이 성공한 뒤 사용자 상태를 변경한다.
        user->EnterRoom(newRoomId);

        m_lastRoomId = newRoomId;
        outRoomId = newRoomId;

        return RoomResult::Success;
    }

    RoomResult NLogicLib::RoomManager::LeaveRoom(NServerNetLib::SessionId sessionId,
        Protocol::GameEndReason reason,
        GameClock::time_point now)
    {
        if (reason != Protocol::GameEndReason::LeftRoom && reason != Protocol::GameEndReason::Disconnected)
            throw std::invalid_argument( "Invalid reason for leaving a room.");

    User* user = m_users.FindMutable(sessionId);

    if (user == nullptr)
        return RoomResult::UserNotFound;

    if (user->GetState() == UserState::Lobby)
    {
        if (user->GetRoomId() != INVALID_ROOM_ID)
            return RoomResult::StateMismatch;

        return RoomResult::NotInRoom;
    }

    const RoomId roomId = user->GetRoomId();
    const auto roomIt = m_rooms.find(roomId);

    if (roomIt == m_rooms.end())
        return RoomResult::StateMismatch;

    Room& room = roomIt->second;

    if (!room.Contains(sessionId))
        return RoomResult::StateMismatch;

    // 이미 제한 시간을 넘었으면 시간 초과 결과가 먼저 확정된다.
    ExpireRoom(room, now);

    if (room.GetPhase() == Protocol::RoomPhase::Playing)
    {
        const Stone loser = room.GetPlayerStone(sessionId);

        if (!room.Forfeit(loser))
            return RoomResult::StateMismatch;

        // 참가자 제거 전에 결과와 전달 대상을 보관한다.
        FinishRoom(room, reason);
    }

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

    

    void RoomManager::UpdateTimeouts(GameClock::time_point now)
    {
        for (auto& entry : m_rooms)
        {
            ExpireRoom(entry.second, now);
        }
    }

    bool RoomManager::TryPopFinishedGame(FinishedGame& out)
    {
        if (m_finishedGames.empty())
            return false;

        out = std::move(m_finishedGames.front());
        m_finishedGames.pop_front();

        return true;
    }

    RoomResult RoomManager::StartGame(NServerNetLib::SessionId sessionId, GameClock::time_point now)
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

        if (m_lastGameId ==(std::numeric_limits<Protocol::GameId>::max)())
            return RoomResult::GameIdExhausted;

        const Protocol::GameId newGameId = m_lastGameId + 1;

        room.StartGame(newGameId, now);

        m_lastGameId = newGameId;

        return RoomResult::Success;
    }

    Protocol::GameMoveResult RoomManager::PlaceStone(NServerNetLib::SessionId sessionId, RoomId requestedRoomId,Protocol::GameId requestedGameId, int x, int y, AcceptedMove& out, GameClock::time_point now)
    {
        using Result = Protocol::GameMoveResult;

        const User* user = m_users.Find(sessionId);

        if (user == nullptr)
            return Result::NotLoggedIn;

        if (user->GetState() != UserState::InRoom)
            return Result::NotInRoom;

        if (user->GetRoomId() != requestedRoomId)
            return Result::WrongRoom;

        const auto it = m_rooms.find(user->GetRoomId());

        if (it == m_rooms.end() || !it->second.Contains(sessionId))
            return Result::StateMismatch;

        Room& room = it->second;

        if (requestedGameId == 0 || requestedGameId != room.GetGameId())
            return Protocol::GameMoveResult::StaleGame;

        if (ExpireRoom(room, now))
            return Protocol::GameMoveResult::GameNotRunning;
        
        if (ExpireRoom(room, now))
            return Protocol::GameMoveResult::GameNotRunning;

        const Stone stone = room.GetPlayerStone(sessionId);

        const MoveResult result =room.PlaceStone(sessionId, x, y,now);

        switch (result)
        {
        case MoveResult::GameNotRunning:
            return Result::GameNotRunning;

        case MoveResult::OutOfBounds:
            return Result::OutOfBounds;

        case MoveResult::NotYourTurn:
            return Result::NotYourTurn;

        case MoveResult::Occupied:
            return Result::Occupied;

        case MoveResult::InvalidStone:
            return Result::StateMismatch;

        case MoveResult::Success:
            break;

        default:
            return Result::StateMismatch;
        }

        AcceptedMove accepted;
        accepted.Room = room.GetId();
        accepted.X = x;
        accepted.Y = y;
        accepted.PlacedStone = stone;

        accepted.NextTurn = room.GetGame().GetNextTurn();
        accepted.Status = room.GetGame().GetStatus();
        accepted.MoveCount = room.GetGame().GetMoveCount();

        accepted.Game = room.GetGameId();

        out = accepted;

        if (accepted.Status == GameStatus::BlackWon || accepted.Status == GameStatus::WhiteWon)
        {
            FinishRoom(room,Protocol::GameEndReason::FiveInRow);
        }
        else if (accepted.Status == GameStatus::Draw)
        {
            FinishRoom(room,Protocol::GameEndReason::BoardFull);
        }

        return Protocol::GameMoveResult::Success;
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
    void RoomManager::FinishRoom(Room& room, Protocol::GameEndReason reason)
    {
        if (room.GetPhase() != Protocol::RoomPhase::Playing)
            return;

        const auto status = room.GetGame().GetStatus();

        if (status != GameStatus::BlackWon &&status != GameStatus::WhiteWon &&status != GameStatus::Draw)
        {
            throw std::logic_error(
                "Cannot finish a game without a final result.");
        }

        FinishedGame finished;
        finished.Room = room.GetId();
        finished.Status = status;
        finished.Reason = reason;
        finished.MoveCount = room.GetGame().GetMoveCount();
        finished.Targets = room.GetMembers();

        finished.Game = room.GetGameId();

        m_finishedGames.push_back(std::move(finished));

        room.FinishGame();
    }

    bool RoomManager::ExpireRoom(Room& room, GameClock::time_point now)
    {
        if (!room.IsTurnExpired(now))
            return false;

        const Stone loser = room.GetGame().GetNextTurn();

        if (!room.Forfeit(loser))
            throw std::logic_error( "Could not resolve expired turn.");

        FinishRoom(room,Protocol::GameEndReason::TurnTimeout);

        return true;
    }

    Protocol::GameMoveResult NLogicLib::RoomManager::Resign(
        NServerNetLib::SessionId sessionId,
        RoomId requestedRoomId, Protocol::GameId requestedGameId,
        GameClock::time_point now)
    {
        using Result = Protocol::GameMoveResult;

        const User* user = m_users.Find(sessionId);

        if (user == nullptr)
            return Result::NotLoggedIn;

        if (user->GetState() != UserState::InRoom)
            return Result::NotInRoom;

        if (user->GetRoomId() != requestedRoomId)
            return Result::WrongRoom;

        const auto it = m_rooms.find(user->GetRoomId());

        if (it == m_rooms.end() ||
            !it->second.Contains(sessionId))
        {
            return Result::StateMismatch;
        }

        Room& room = it->second;

        if (requestedGameId == 0 ||
            requestedGameId != room.GetGameId())
        {
            return Protocol::GameMoveResult::StaleGame;
        }

        if (ExpireRoom(room, now))
        {
            return Protocol::GameMoveResult::GameNotRunning;
        }

        if (room.GetPhase() != Protocol::RoomPhase::Playing ||
            room.GetGame().GetStatus() != GameStatus::Playing)
        {
            return Result::GameNotRunning;
        }

        const Stone loser = room.GetPlayerStone(sessionId);

        if (!room.Forfeit(loser))
        {
            return Result::StateMismatch;
        }

        FinishRoom(
            room,
            Protocol::GameEndReason::Resigned);

        return Result::Success;
    }
}