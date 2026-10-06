#pragma once
#include <string>
#include <unordered_map>
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
		RoomResult StartGame(NServerNetLib::SessionId sessionId);

		Protocol::GameMoveResult PlaceStone(NServerNetLib::SessionId sessionId,RoomId requestedRoomId,int x,int y,AcceptedMove& out);
	private:
		bool IsValidTitle(const std::string& title) const;
	private:
		UserManager& m_users;
		RoomConfig m_config;

		std::unordered_map<RoomId, Room> m_rooms;

		RoomId m_lastRoomId = INVALID_ROOM_ID;
	};

}