
#include "PacketProcessor.h"
#include "../Common/PacketID.h"
#include "../Common/PacketUtils.h"
#include "../../Common/PacketCode.h"
#include <Windows.h>

#include <cstring>
#include <iostream>
#include <string>
#include <vector>


void NLogicLib::PacketProcessor::HandleChat(const NServerNetLib::NetworkEvent& event)
{
	//1. 발신자 의 로그인 여부 확인
	const User* sender = m_users.Find(event.Session);

	if (sender == nullptr)
	{
		SendChatResult(event.Session, Protocol::ChatResult::NotLoggedIn);
		return;
	}

	if (sender->GetState() != UserState::InRoom)
	{
		SendChatResult(event.Session,Protocol::ChatResult::NotInRoom);
		return;
	}

	const RoomId roomId = sender->GetRoomId();
	const Room* room = m_rooms.Find(roomId);

	if (room == nullptr || !room->Contains(event.Session))
	{
		std::cerr<< "[Chat] User/room state mismatch. Session: "<< event.Session<< '\n';

		SendChatResult(event.Session,Protocol::ChatResult::StateMismatch);
		return;
	}


	// 발신자의 이름은 서버의 사용자 정보에서 가져옴
	Protocol::ChatRequest request;

	if (false == Protocol::Decode(event.Body, request))
	{
		m_network.Disconnect(event.Session);
		return;
	}
	//2. 메시지 검사
	if (!IsValidChatMessage(request.Message))
	{
		SendChatResult(event.Session, Protocol::ChatResult::InvalidMessage);
		return;
	}
	//3. 채팅 알림 본문 구성
	// [닉네임 길이 1][닉네임][메시지 길이 2][메시지]

	Protocol::ChatNotification notification;
	notification.Nickname = sender->Nickname;
	notification.Message = request.Message;

	Protocol::PacketBody body;

	if (!Protocol::Encode(notification, body))
	{
		std::cerr << "Could not encode chat notification.\n";
		m_network.Disconnect(event.Session);

		return;
	}

	//4. 요청자에게 요청 수락 결과 전달
	if (!SendChatResult(event.Session, Protocol::ChatResult::Success))
		return;


	//5. 현제 로그인 사용자들의 세션 목록
	const auto targets = room->GetMembers();

	if (!SendChatResult(event.Session,Protocol::ChatResult::Success))
		return;

	//6. 발신지를 포함한 로그인 사용자에게 알림
	for (SessionId target : targets)
	{
		if (!m_network.IsConnected(target))
			continue;

		const bool queued = m_network.SendPacket(target, Protocol::CHAT_NTF, body.data(), body.size());
	
		if (!queued)
		{
			// 이 연결은 더 이상 정상적으로 보내기 어려움
			// 종료 이벤트를 통해 사용자 정보도 정리됨
			m_network.Disconnect(target);
		}
	}



	std::cout << "[Chat] " << notification.Nickname
		<< ", message bytes: "
		<< body.size()
		<< '\n';
}

bool NLogicLib::PacketProcessor::SendChatResult(SessionId sessionId, Protocol::ChatResult result)
{
	//채팅 요청을 보낸 사람에게 ‘처리 결과’를 보내는 함수

	Protocol::ChatResponse response;
	response.Result = result;

	const auto body = Protocol::Encode(response);

	const bool queued = m_network.SendPacket(sessionId, Protocol::CHAT_RES, body.data(),body.size());

	if (!queued)
		m_network.Disconnect(sessionId);

	return queued;
}

bool NLogicLib::PacketProcessor::IsValidChatMessage(const std::string& message) const
{
	if (message.empty() || message.size() > Protocol::MAX_CHAT_MESSAGE_BYTES)
		return false;

	bool hasNonSpace = false;

	for (unsigned char ch : message)
	{
		// NUL, 줄바꿈, 탭 등 ASCII 제어 문자 거절
		if (ch < 0x20 || ch == 0x7F)
			return false;


		if (ch != ' ')
			hasNonSpace = true;
	}

	if (!hasNonSpace)
		return false;

	// 올바른 UTF-8인지 Windows API로 검사
	const int requiredCharacters = MultiByteToWideChar(
		CP_UTF8,
		MB_ERR_INVALID_CHARS,
		message.data(),
		static_cast<int>(message.size()),
		nullptr,
		0
	);

	return requiredCharacters > 0;
}
