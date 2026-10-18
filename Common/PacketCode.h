#pragma once

#include <vector>

#include "PacketMessages.h"
#include "PacketUtils.h"


//패킷 종류가 늘어나면, 그 패킷의 구조체와 Encode()·Decode()도 보통 추가

namespace Protocol
{
	using PacketBody = std::vector<char>;

	// LOGIN_REQ: 닉네임 바이트
	inline PacketBody Encode(const LoginRequest& packet)
	{
		return PacketBody(packet.Nickname.begin(), packet.Nickname.end());
	}

	inline bool Decode(const PacketBody& body, LoginRequest& out)
	{
		if (body.size() > MAX_PACKET_SIZE - HEADER_SIZE)
			return false;

		out.Nickname.assign(body.begin(), body.end());
		return true;
	}

	// LOGIN_RES: 결과 코드 1바이트
	inline PacketBody Encode(const LoginResponse& packet)
	{
		return PacketBody{ static_cast<char>(packet.Result) };
	}

	inline bool Decode(const PacketBody& body, LoginResponse& out)
	{
		if (body.size() != 1)
			return false;

		const auto value = static_cast<unsigned char>(body[0]);

		switch (static_cast<LoginResult>(value))
		{
		case LoginResult::Success:
		case LoginResult::InvalidNickname:
		case LoginResult::NicknameInUse:
		case LoginResult::AlreadyLoggedIn:
			out.Result = static_cast<LoginResult>(value);
			return true;

		default:
			return false;
		}

	}

	// CHAT_REQ: 메시지 바이트
	inline PacketBody Encode(const ChatRequest& packet)
	{
		return PacketBody(packet.Message.begin(), packet.Message.end());
	}

	inline bool Decode(const PacketBody& body, ChatRequest& out)
	{
		if (body.size() > MAX_PACKET_SIZE - HEADER_SIZE)
			return false;

		out.Message.assign(body.begin(), body.end());

		return true;
	}

	// CHAT_RES: 결과 코드 1바이트

	inline PacketBody Encode(const ChatResponse& packet)
	{
		return PacketBody{static_cast<char>(packet.Result)};
	}

	inline bool Decode(const PacketBody& body,ChatResponse& out)
	{
		if (body.size() != 1)
		{
			return false;
		}

		const auto value = static_cast<unsigned char>(body[0]);

		switch (static_cast<ChatResult>(value))
		{
		case ChatResult::Success:
		case ChatResult::NotLoggedIn:
		case ChatResult::InvalidMessage:
		case ChatResult::NotInRoom:
		case ChatResult::StateMismatch:
			out.Result = static_cast<ChatResult>(value);
			return true;

		default:
			return false;
		}
	}

	// CHAT_NTF:
    inline bool Encode(const ChatNotification& packet,PacketBody& out)
    {
        const size_t nicknameSize = packet.Nickname.size();
        const size_t messageSize = packet.Message.size();

        if (nicknameSize == 0 || nicknameSize > MAX_NICKNAME_BYTES ||  nicknameSize > 255)
            return false;

        if (messageSize == 0 || messageSize > MAX_CHAT_MESSAGE_BYTES || messageSize > 65535)
            return false;

        const size_t totalBodySize = 1 + nicknameSize + 2 + messageSize;

        if (totalBodySize > MAX_PACKET_SIZE - HEADER_SIZE)
            return false;

        PacketBody encoded(totalBodySize);

        size_t pos = 0;

        encoded[pos++] = static_cast<char>(nicknameSize);

        for (char ch : packet.Nickname)
        {
            encoded[pos++] = ch;
        }

        WriteUInt16(encoded.data() + pos,static_cast<UINT16>(messageSize));

        pos += 2;

        for (char ch : packet.Message)
        {
            encoded[pos++] = ch;
        }

        out = std::move(encoded);
        return true;
    }

    inline bool Decode(const PacketBody& body, ChatNotification& out)
    {
        if (body.size() < 3 || body.size() > MAX_PACKET_SIZE - HEADER_SIZE)
            return false;

        size_t pos = 0;

        const size_t nicknameSize = static_cast<unsigned char>(body[pos++]);

        if (nicknameSize == 0 || nicknameSize > MAX_NICKNAME_BYTES)
            return false;

        // 닉네임 뒤의 메시지 길이 필드까지 존재해야 한다.
        if (body.size() - pos < nicknameSize + 2)
            return false;

        ChatNotification decoded;

        decoded.Nickname.assign( body.data() + pos, nicknameSize);

        pos += nicknameSize;

        const std::size_t messageSize = ReadUInt16(body.data() + pos);

        pos += 2;

        if (messageSize == 0 || messageSize > MAX_CHAT_MESSAGE_BYTES)
            return false;

        // 선언된 길이와 실제 남은 바이트가 정확히 일치해야 한다.
        if (body.size() - pos != messageSize)
            return false;

        decoded.Message.assign(body.data() + pos,messageSize);

        out = std::move(decoded);
        return true;
    }
}
