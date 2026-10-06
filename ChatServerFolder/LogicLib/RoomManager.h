#pragma once
#include <string>
#include <unordered_map>
#include <deque>
#include <vector>
#include "Room.h"
#include "RoomTypes.h"
#include "UserManager.h"
#include "../../Common/GamePackets.h"
namespace NLogicLib
{
	struct AcceptedMove
	{
		RoomId Room = INVALID_ROOM_ID;

		int X = 0;
		int Y = 0;

		Stone PlacedStone = Stone::Empty;
		Stone NextTurn = Stone::Empty;

		GameStatus Status = GameStatus::NotStarted;

		size_t MoveCount = 0;
	};

	struct FinishedGame
	{
		RoomId Room = INVALID_ROOM_ID;

		GameStatus Status = GameStatus::NotStarted;

		Protocol::GameEndReason Reason =
			Protocol::GameEndReason::FiveInRow;

		size_t MoveCount = 0;

		std::vector<NServerNetLib::SessionId> Targets;
	};

	class RoomManager
	{
	public:
		RoomManager(UserManager& users, RoomConfig config = {});
		virtual ~RoomManager();

	public:
		RoomResult CreateRoom(NServerNetLib::SessionId sessionId,const std::string& title,RoomId& outRoomId);
		RoomResult LeaveRoom(NServerNetLib::SessionId sessionId);
		const Room* Find(RoomId roomId) const;

		std::size_t GetRoomCount() const { return m_rooms.size(); };

		RoomResult EnterRoom(NServerNetLib::SessionId sessionId,RoomId roomId);

		std::vector<Protocol::RoomInfo> GetRoomsAfter(RoomId afterRoomId,bool& outHasMore) const;

		//
		RoomResult SetReady(NServerNetLib::SessionId sessionId,bool ready);
		RoomResult StartGame(NServerNetLib::SessionId sessionId, GameClock::time_point now = GameClock::now());

		Protocol::GameMoveResult PlaceStone(NServerNetLib::SessionId sessionId,RoomId requestedRoomId,int x,int y,AcceptedMove& out, GameClock::time_point now = GameClock::now());
		Protocol::GameMoveResult Resign(NServerNetLib::SessionId sessionId,RoomId requestedRoomId,GameClock::time_point now = GameClock::now());
		void UpdateTimeouts(GameClock::time_point now = GameClock::now());
		bool TryPopFinishedGame(FinishedGame& out);
	private:
		bool IsValidTitle(const std::string& title) const;

		void FinishRoom(Room& room,Protocol::GameEndReason reason);
		bool ExpireRoom(Room& room,GameClock::time_point now);
	private:
		UserManager& m_users;
		RoomConfig m_config;

		std::unordered_map<RoomId, Room> m_rooms;

		RoomId m_lastRoomId = INVALID_ROOM_ID;

		std::deque<FinishedGame> m_finishedGames;
	};

}