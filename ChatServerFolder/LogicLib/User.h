#pragma once

#include <string>
#include "../SeverNetLib/NetworkEvent.h"

#include "RoomTypes.h"

namespace NLogicLib
{
	class RoomManager;

	class User
	{
	public:

		NServerNetLib::SessionId Session = 0;
		std::string Nickname;

		UserState GetState() const
		{
			return m_state;
		}

		RoomId GetRoomId() const
		{
			return m_roomId;
		}

	private:

		friend class RoomManager;

		void EnterRoom(RoomId roomId)
		{
			m_roomId = roomId;
			m_state = UserState::InRoom;
		}

		void ReturnToLobby()
		{
			m_roomId = INVALID_ROOM_ID;
			m_state = UserState::Lobby;
		}

	private:
		UserState m_state = UserState::Lobby;

		RoomId m_roomId = INVALID_ROOM_ID;

	};
}