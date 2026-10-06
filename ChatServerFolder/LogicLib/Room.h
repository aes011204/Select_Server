#pragma once
#include <string>
#include <vector>
#include "../SeverNetLib/NetworkEvent.h"
#include "RoomTypes.h"
#include "OmokGame.h"
#include <unordered_set>

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


		//
		NServerNetLib::SessionId GetHostSession() const { return m_hostSession; };
		Protocol::RoomPhase GetPhase() const { return m_phase; };
		bool IsReady(NServerNetLib::SessionId sessionId) const {
			return m_readyMembers.find(sessionId) != m_readyMembers.end();};

		const OmokGame& GetGame() const { return m_game; };
		Stone GetPlayerStone(NServerNetLib::SessionId sessionId) const;
	private:
		friend class RoomManager;
		bool RemoveMember(NServerNetLib::SessionId sessionId);

		bool AddMember(NServerNetLib::SessionId sessionId);

		//
		void SetReady(NServerNetLib::SessionId sessionId,bool ready);
		void StartGame();

		MoveResult PlaceStone(NServerNetLib::SessionId sessionId,int x,int y);

	private:
		RoomId m_id;
		std::string m_title;
		size_t m_capacity;

		std::vector<NServerNetLib::SessionId> m_members;

		//
		NServerNetLib::SessionId m_hostSession = 0;
		Protocol::RoomPhase m_phase =Protocol::RoomPhase::Waiting;
		std::unordered_set<NServerNetLib::SessionId> m_readyMembers;

		OmokGame m_game;
		NServerNetLib::SessionId m_blackPlayer = 0;
		NServerNetLib::SessionId m_whitePlayer = 0;
	};

}