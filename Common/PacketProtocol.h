#pragma once


namespace Protocol
{

	constexpr size_t HEADER_SIZE = 4;

	//헤더를 포함한 패킷하나의 최대 크기
	constexpr size_t MAX_PACKET_SIZE = 4096;

	enum class LoginResult
	{
		Success = 0,
		InvalidNickname = 1,
		NicknameInUse = 2,
		AlreadyLoggedIn = 3
	};

	constexpr std::size_t MAX_NICKNAME_BYTES = 16;

	enum class ChatResult : std::uint8_t
	{
		Success = 0,
		NotLoggedIn = 1,
		InvalidMessage = 2,

		NotInRoom = 3,
		StateMismatch = 4
	};

	constexpr std::size_t MAX_CHAT_MESSAGE_BYTES = 256;
}