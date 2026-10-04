#pragma once
#include <windows.h>
#include <string>
#include <vector>
namespace Protocol
{

    constexpr size_t MAX_ROOM_TITLE_BYTES = 48;
    constexpr size_t ROOM_LIST_PAGE_SIZE = 20;

    enum class RoomResult : UINT8
    {
        Success = 0,

        UserNotFound = 1,
        AlreadyInRoom = 2,
        NotInRoom = 3,

        InvalidTitle = 4,
        RoomLimitReached = 5,
        RoomIdExhausted = 6,

        StateMismatch = 7,

        RoomNotFound = 8,
        RoomFull = 9,
        InvalidRequest = 10,

        NotHost = 11,
        HostCannotReady = 12,
        NotEnoughPlayers = 13,
        NotAllReady = 14,
        GameAlreadyStarted = 15,
        GameInProgress = 16
    };



    struct RoomCreateRequest
    {
        std::string Title;
    };

    // 생성 응답과 입장 응답은 본문 형식이 같다.
    struct RoomActionResponse
    {
        RoomResult Result = RoomResult::Success;
        std::uint32_t RoomId = 0;
    };

    struct RoomListRequest
    {
        std::uint32_t AfterRoomId = 0;
    };

    struct RoomInfo
    {
        UINT32 RoomId = 0;
        UINT8 UserCount = 0;
        UINT8 Capacity = 0;
        std::string Title;
    };

    struct RoomListResponse
    {
        RoomResult Result = RoomResult::Success;
        bool HasMore = false;
        std::vector<RoomInfo> Rooms;
    };

    struct RoomEnterRequest
    {
        std::uint32_t RoomId = 0;
    };

    struct RoomLeaveRequest
    {
    };

    enum class RoomMemberChange : std::uint8_t
    {
        Joined = 1,
        Left = 2,
        Disconnected = 3
    };

    struct RoomMemberNotification
    {
        std::uint32_t RoomId = 0;

        RoomMemberChange Change =
            RoomMemberChange::Joined;

        std::string Nickname;
    };

    // 준비
    constexpr size_t ROOM_PLAYER_COUNT = 2;

    enum class RoomPhase : UINT8
    {
        Waiting = 0,
        Playing = 1
    };

    struct RoomReadyRequest
    {
        bool Ready = false;
    };

    struct RoomStartRequest
    {
    };

    struct RoomPlayerInfo
    {
        std::string Nickname;
        bool Ready = false;
    };

    struct RoomStateNotification
    {
        UINT32 RoomId = 0;

        RoomPhase Phase = RoomPhase::Waiting;
        UINT8 Capacity = 2;

        std::string HostNickname;
        std::vector<RoomPlayerInfo> Players;
    };
}