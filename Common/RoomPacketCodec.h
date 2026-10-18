#pragma once


#include <string>
#include <unordered_set>
#include "PacketCode.h"
#include "RoomPackets.h"

namespace Protocol
{
	namespace RoomCodecDetail
	{
		class Writer
		{
		public:
			void U8(UINT8 value)//0~255
			{
				Body.push_back(static_cast<char>(value));
			}

			void U32(UINT32 value)
			{
				for (int shift = 24; shift >= 0; shift -= 8)
				{
					U8(static_cast<std::uint8_t>((value >> shift) & 0xFF));
				}
			}

			bool Text8(const std::string& text)
			{
				if (text.size() > 255)
					return false;
				
				U8(static_cast<UINT8>(text.size()));

				Body.insert(Body.end(), text.begin(), text.end());

				return true;
			}

			bool Finish(PacketBody& out)
			{
				if (Body.size() > MAX_PACKET_SIZE - HEADER_SIZE)
					return false;

				out = Body;
				return true;
			}
		private:
			PacketBody Body;
		};

		class Reader
		{
		public:
			Reader(const PacketBody& body)
				: m_body(body)
			{
			}
			bool U8(UINT8& out)
			{
				if (m_pos >= m_body.size())
					return false;

				out = static_cast<unsigned char>(m_body[m_pos++]);
				return true;
			}

			bool U32(UINT32& out)
			{
				if (m_body.size() - m_pos < 4)
					return false;

				UINT32 value = 0;

				for (int i = 0; i < 4; ++i)
				{
					value = (value << 8) |static_cast<unsigned char>(m_body[m_pos++]);
				}
			
				out = value;

				return true;
			}

			bool Text8(std::string& out)
			{
				UINT8 size = 0;

				if (!U8(size))
					return false;

				if (m_body.size() - m_pos < size)
					return false;

				out.assign(m_body.begin() + m_pos,m_body.begin() + m_pos + size);

				m_pos += size;

				return true;
			}

			bool Done() const
			{
				return m_pos == m_body.size();
			}

		private:
			const PacketBody& m_body;
			size_t m_pos = 0;
		};

		inline bool ReadResult(Reader& reader, RoomResult& out)
		{
			UINT8 value = 0;

			if (!reader.U8(value) ||
				value > static_cast<UINT8>(RoomResult::GameIdExhausted))
			{
				return false;
			}

			out = static_cast<RoomResult>(value);
			return true;
		}
	}

	// 방 생성 요청
	inline bool Encode(const RoomCreateRequest& packet,PacketBody& out)
	{
		RoomCodecDetail::Writer writer;

		if (!writer.Text8(packet.Title))
			return false;

		return writer.Finish(out);
	}

	inline bool Decode(const PacketBody& body,RoomCreateRequest& out)
	{
		RoomCodecDetail::Reader reader(body);
		RoomCreateRequest decoded;

		if (!reader.Text8(decoded.Title) || !reader.Done())
			return false;

		out = std::move(decoded);
		return true;
	}

	//생성·입장 응답
	inline bool Encode(const RoomActionResponse& packet,PacketBody& out)
	{
		RoomCodecDetail::Writer writer;

		writer.U8(static_cast<UINT8>(packet.Result));
		writer.U32(packet.RoomId);

		return writer.Finish(out);
	}

	inline bool Decode(const PacketBody& body,RoomActionResponse& out)
	{
		RoomCodecDetail::Reader reader(body);
		RoomActionResponse decoded;

		if (!RoomCodecDetail::ReadResult(reader, decoded.Result) ||
			!reader.U32(decoded.RoomId) ||
			!reader.Done())
		{
			return false;
		}

		const bool success = decoded.Result == RoomResult::Success;

		if ((success && decoded.RoomId == 0) ||
			(!success && decoded.RoomId != 0))
		{
			return false;
		}

		out = decoded;
		return true;
	}

	//목록 요청
	inline bool Encode(const RoomListRequest& packet,PacketBody& out)
	{
		RoomCodecDetail::Writer writer;
		writer.U32(packet.AfterRoomId);
		return writer.Finish(out);
	}

	inline bool Decode(const PacketBody& body,RoomListRequest& out)
	{
		RoomCodecDetail::Reader reader(body);
		RoomListRequest decoded;

		if (!reader.U32(decoded.AfterRoomId) || !reader.Done())
			return false;

		out = decoded;
		return true;
	}

	//입장 요청
	inline bool Encode(const RoomEnterRequest& packet,PacketBody& out)
	{
		RoomCodecDetail::Writer writer;
		writer.U32(packet.RoomId);
		return writer.Finish(out);
	}

	inline bool Decode(const PacketBody& body,RoomEnterRequest& out)
	{
		RoomCodecDetail::Reader reader(body);
		RoomEnterRequest decoded;

		if (!reader.U32(decoded.RoomId) || !reader.Done())
			return false;

		out = decoded;
		return true;
	}

	//목록 응답
	inline bool Encode(const RoomListResponse& packet,PacketBody& out)
	{
		if (packet.Rooms.size() > ROOM_LIST_PAGE_SIZE)
			return false;

		if (packet.Result != RoomResult::Success &&
			(!packet.Rooms.empty() || packet.HasMore))
			return false;

		if (packet.HasMore && packet.Rooms.empty())
			return false;

		RoomCodecDetail::Writer writer;

		writer.U8(static_cast<UINT8>(packet.Result));
		writer.U8(packet.HasMore ? 1 : 0);
		writer.U8(static_cast<UINT8>(packet.Rooms.size()));

		std::uint32_t previousId = 0;

		for (const auto& room : packet.Rooms)
		{
			if (room.RoomId <= previousId ||
				room.Capacity == 0 ||
				room.UserCount > room.Capacity ||
				room.Title.empty() ||
				room.Title.size() > MAX_ROOM_TITLE_BYTES)
			{
				return false;
			}

			previousId = room.RoomId;

			writer.U32(room.RoomId);
			writer.U8(room.UserCount);
			writer.U8(room.Capacity);

			if (!writer.Text8(room.Title))
				return false;
		}

		return writer.Finish(out);
	}

	inline bool Decode(const PacketBody& body,RoomListResponse& out)
	{
		RoomCodecDetail::Reader reader(body);
		RoomListResponse decoded;

		UINT8 hasMore = 0;
		UINT8 count = 0;

		if (!RoomCodecDetail::ReadResult(reader, decoded.Result) ||
			!reader.U8(hasMore) ||
			!reader.U8(count))
		{
			return false;
		}

		if (hasMore > 1 || count > ROOM_LIST_PAGE_SIZE)
			return false;

		decoded.HasMore = hasMore != 0;

		if (decoded.Result != RoomResult::Success &&(count != 0 || decoded.HasMore))
			return false;

		if (decoded.HasMore && count == 0)
			return false;

		UINT32 previousId = 0;

		for (UINT8 i = 0; i < count; ++i)
		{
			RoomInfo room;

			if (!reader.U32(room.RoomId) ||
				!reader.U8(room.UserCount) ||
				!reader.U8(room.Capacity) ||
				!reader.Text8(room.Title))
			{
				return false;
			}

			if (room.RoomId <= previousId ||
				room.Capacity == 0 ||
				room.UserCount > room.Capacity ||
				room.Title.empty() ||
				room.Title.size() > MAX_ROOM_TITLE_BYTES)
			{
				return false;
			}

			previousId = room.RoomId;
			decoded.Rooms.push_back(std::move(room));
		}

		if (!reader.Done())
			return false;

		out = std::move(decoded);
		return true;
	}

	//퇴장
	inline bool Encode(const RoomLeaveRequest&,PacketBody& out)
	{
		out.clear();
		return true;
	}

	inline bool Decode(const PacketBody& body,RoomLeaveRequest&)
	{
		return body.empty();
	}

	//참가자 알림
	inline bool Encode(
		const RoomMemberNotification& packet,PacketBody& out)
	{
		if (packet.RoomId == 0 ||
			packet.Nickname.empty() ||
			packet.Nickname.size() > MAX_NICKNAME_BYTES)
		{
			return false;
		}

		switch (packet.Change)
		{
		case RoomMemberChange::Joined:
		case RoomMemberChange::Left:
		case RoomMemberChange::Disconnected:
			break;

		default:
			return false;
		}

		RoomCodecDetail::Writer writer;

		writer.U32(packet.RoomId);
		writer.U8(static_cast<std::uint8_t>(packet.Change));

		if (!writer.Text8(packet.Nickname))
		{
			return false;
		}

		return writer.Finish(out);
	}

	inline bool Decode(
		const PacketBody& body,
		RoomMemberNotification& out)
	{
		RoomCodecDetail::Reader reader(body);
		RoomMemberNotification decoded;

		std::uint8_t change = 0;

		if (!reader.U32(decoded.RoomId) ||
			!reader.U8(change) ||
			!reader.Text8(decoded.Nickname) ||
			!reader.Done())
		{
			return false;
		}

		if (decoded.RoomId == 0 ||
			decoded.Nickname.empty() ||
			decoded.Nickname.size() > MAX_NICKNAME_BYTES)
		{
			return false;
		}

		switch (static_cast<RoomMemberChange>(change))
		{
		case RoomMemberChange::Joined:
		case RoomMemberChange::Left:
		case RoomMemberChange::Disconnected:
			decoded.Change =
				static_cast<RoomMemberChange>(change);
			break;

		default:
			return false;
		}

		out = std::move(decoded);
		return true;
	}

	//
	inline bool Encode(const RoomReadyRequest& packet,PacketBody& out)
	{
		RoomCodecDetail::Writer writer;
		writer.U8(packet.Ready ? 1 : 0);
		return writer.Finish(out);
	}

	inline bool Decode(const PacketBody& body,RoomReadyRequest& out)
	{
		RoomCodecDetail::Reader reader(body);

		std::uint8_t ready = 0;

		if (!reader.U8(ready) ||ready > 1 ||!reader.Done())
		{
			return false;
		}

		out.Ready = ready != 0;
		return true;
	}

	inline bool Encode(const RoomStartRequest&,PacketBody& out)
	{
		out.clear();
		return true;
	}

	inline bool Decode(const PacketBody& body,RoomStartRequest&)
	{
		return body.empty();
	}

	//
	inline bool IsValidRoomState(
		const RoomStateNotification& packet)
	{
		if (packet.RoomId == 0 ||
			packet.Capacity != ROOM_PLAYER_COUNT ||
			packet.Players.empty() ||
			packet.Players.size() > ROOM_PLAYER_COUNT ||
			packet.HostNickname.empty() ||
			packet.HostNickname.size() > MAX_NICKNAME_BYTES)
		{
			return false;
		}

		if (packet.Phase != RoomPhase::Waiting &&packet.Phase != RoomPhase::Playing)
			return false;

		if (packet.Phase == RoomPhase::Playing &&packet.Players.size() != ROOM_PLAYER_COUNT)
			return false;

		if (packet.Phase == RoomPhase::Playing && packet.Game == 0)
		{
			return false;
		}

		std::unordered_set<std::string> names;
		bool hostFound = false;

		for (const auto& player : packet.Players)
		{
			if (player.Nickname.empty() ||
				player.Nickname.size() > MAX_NICKNAME_BYTES ||
				!names.insert(player.Nickname).second)
			{
				return false;
			}

			if (player.Nickname == packet.HostNickname)
			{
				hostFound = true;

				if (player.Ready)
					return false;
			}

			if (packet.Phase == RoomPhase::Playing &&player.Ready)
				return false;
		}

		return hostFound;
	}

	inline bool Encode(
		const RoomStateNotification& packet,
		PacketBody& out)
	{
		if (!IsValidRoomState(packet))
			return false;

		RoomCodecDetail::Writer writer;

		writer.U32(packet.RoomId);
		writer.U32(packet.Game);
		writer.U8(static_cast<std::uint8_t>(packet.Phase));
		writer.U8(packet.Capacity);

		if (!writer.Text8(packet.HostNickname))
			return false;

		writer.U8(static_cast<std::uint8_t>(packet.Players.size()));

		for (const auto& player : packet.Players)
		{
			if (!writer.Text8(player.Nickname))
			{
				return false;
			}

			writer.U8(player.Ready ? 1 : 0);
		}

		return writer.Finish(out);
	}

	inline bool Decode(
		const PacketBody& body,
		RoomStateNotification& out)
	{
		RoomCodecDetail::Reader reader(body);
		RoomStateNotification decoded;

		UINT8 phase = 0;
		UINT8 count = 0;

		if (!reader.U32(decoded.RoomId) ||
			!reader.U32(decoded.Game) ||
			!reader.U8(phase) ||
			!reader.U8(decoded.Capacity) ||
			!reader.Text8(decoded.HostNickname) ||
			!reader.U8(count))
		{
			return false;
		}

		if (count == 0 || count > ROOM_PLAYER_COUNT)
			return false;

		decoded.Phase = static_cast<RoomPhase>(phase);

		for (UINT8 i = 0; i < count; ++i)
		{
			RoomPlayerInfo player;
			UINT8 ready = 0;

			if (!reader.Text8(player.Nickname) ||
				!reader.U8(ready) ||
				ready > 1)
			{
				return false;
			}

			player.Ready = ready != 0;
			decoded.Players.push_back(std::move(player));
		}

		if (!reader.Done() || !IsValidRoomState(decoded))
			return false;

		out = std::move(decoded);
		return true;
	}
}
