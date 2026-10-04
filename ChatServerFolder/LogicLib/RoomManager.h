#pragma once
#include <string>
#include <unordered_map>

#include "Room.h"
#include "RoomTypes.h"
#include "UserManager.h"
namespace NLogicLib
{
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
	private:
		bool IsValidTitle(const std::string& title) const;
	private:
		UserManager& m_users;
		RoomConfig m_config;

		std::unordered_map<RoomId, Room> m_rooms;

		RoomId m_lastRoomId = INVALID_ROOM_ID;
	};

}