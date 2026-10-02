#pragma once
#include "../SeverNetLib/NetworkEvent.h"
#include "UserManager.h"
#include "../SeverNetLib/INetwork.h"
#include <functional>
#include <array>
#include <chrono>
#include <unordered_map>
#include "RoomManager.h"

namespace NLogicLib
{
	using SessionId = NServerNetLib::SessionId;

	class PacketProcessor
	{
	public:
		PacketProcessor(NServerNetLib::INetwork& network, std::chrono::seconds loginTimeout =
			std::chrono::seconds{ 10 });
		//람다가 [this]로 현재 객체의 주소를 기억하기 때문 그 객체를 복사하거나 이동하면, 복사된 람다가 여전히 원래 객체를 가리킬 수 있음
		PacketProcessor(const PacketProcessor&) = delete;
		PacketProcessor& operator=(const PacketProcessor&) = delete;
		PacketProcessor(PacketProcessor&&) = delete;
		PacketProcessor& operator=(PacketProcessor&&) = delete;
	public:
		void Update();
	private:
		//void Process(const ReceivedPacket& packet);

		void ProcessPacket(const NServerNetLib::NetworkEvent& event);
		void HandleLogin(const NServerNetLib::NetworkEvent& event);
		void HandleDisconnected(SessionId sessionId);

		void HandleChat(const NServerNetLib::NetworkEvent& event);
		bool SendChatResult(SessionId sessionId, Protocol::ChatResult result);

		bool IsValidChatMessage(const std::string& message) const;
	private:
		using PacketHandler =
			std::function<void(
				const NServerNetLib::NetworkEvent&)>;

		static constexpr std::size_t HANDLER_COUNT = 256;

		void RegisterHandlers();

		void RegisterHandler(UINT16 packetId,PacketHandler handler);

		//void ProcessPacket(const NServerNetLib::NetworkEvent& event);

		void HandleEcho(const NServerNetLib::NetworkEvent& event);



		// 로그인 대기 시간 
		using Clock = std::chrono::steady_clock;

		void HandleConnected(const NServerNetLib::NetworkEvent& event);
		bool IsLoginExpired(SessionId sessionId) const;
		void CheckLoginTimeouts();

		// 방
		void HandleRoomCreate(const NServerNetLib::NetworkEvent& event);
		void HandleRoomList(const NServerNetLib::NetworkEvent& event);
		void HandleRoomEnter(const NServerNetLib::NetworkEvent& event);
		bool SendRoomActionResult(SessionId sessionId,UINT16 responseId,Protocol::RoomResult result,
			UINT32 roomId);

		//방 채팅
		void NotifyRoomMember(RoomId roomId,Protocol::RoomMemberChange change,const std::string& nickname);
		void HandleRoomLeave(const NServerNetLib::NetworkEvent& event);

	private:
		std::array<PacketHandler, HANDLER_COUNT> m_handlers{};


		NServerNetLib::INetwork& m_network;
		UserManager m_users; 
		RoomManager m_rooms;// 여기서 유저를 참조함 

		// 로그인 대기 시간 
		std::chrono::seconds m_loginTimeout;

		std::unordered_map<SessionId, Clock::time_point> m_loginDeadlines;
	};

}