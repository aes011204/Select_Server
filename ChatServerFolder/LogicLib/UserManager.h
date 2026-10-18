#pragma once

#include <string>
#include <unordered_map>

#include "../../Common/PacketProtocol.h"
#include "User.h"

namespace NLogicLib
{
	class RoomManager;

	class UserManager
	{
	public:
		Protocol::LoginResult Login(NServerNetLib::SessionId sessionId, const std::string& nickname);

		const User* Find(NServerNetLib::SessionId sessionId) const; // 수정 불가 조회

		void Remove(NServerNetLib::SessionId sessionId);

		std::vector<NServerNetLib::SessionId> GetSessionId() const;
	private:
		bool IsValidNickname(const std::string& nickname) const;


		friend class RoomManager;
		User* FindMutable(NServerNetLib::SessionId sessionId); // 수정 가능 조회
	private:
		std::unordered_map<NServerNetLib::SessionId, User> m_users;
		std::unordered_map<std::string, NServerNetLib::SessionId> m_sessionByNickname;

	};

}