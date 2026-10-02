#pragma once
#include <string>
#include <vector>
#include "../SeverNetLib/NetworkEvent.h"
#include "RoomTypes.h"

namespace NLogicLib
{
	class RoomManager;

	class Room
	{
	public:
		Room(RoomId id, std::string title, size_t capacity, NServerNetLib::SessionId creator);

		virtual ~Room();

	public:
		RoomId GetId() const { return m_id; };
		const std::string GetTitle() const { return m_title; };

		size_t GetCapacity()const { return m_capacity; };
		size_t GetUserCount()const {return m_members.size();};

		bool Contains(NServerNetLib::SessionId sessionId) const;
		bool IsEmpty() const { return m_members.empty(); };
		const std::vector<NServerNetLib::SessionId>& GetMembers() const { return m_members; };

	private:
		friend class RoomManager;
		bool RemoveMember(NServerNetLib::SessionId sessionId);

		bool AddMember(NServerNetLib::SessionId sessionId);

	private:
		RoomId m_id;
		std::string m_title;
		size_t m_capacity;

		std::vector<NServerNetLib::SessionId> m_members;
	};

}