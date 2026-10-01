#pragma once
//가짜 네트워크 테스트는 실제 TcpNetwork를 생성하지 않음.
// 따라서 이 코드 자체에는 Winsock 함수 호출이나 Ws2_32.lib 링크가 필요하지 않음
#include <winsock2.h>

#include "../ChatServerFolder/SeverNetLib/INetwork.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <unordered_set>
#include <utility>
#include <vector>

class FakeNetwork final : public NServerNetLib::INetwork
{
public:
	using SessionId = NServerNetLib::SessionId;
	using Event = NServerNetLib::NetworkEvent;
	using EventType = NServerNetLib::NetworkEventType;
	using Clock = std::chrono::steady_clock;


    struct SentPacket
    {
        SessionId Session = 0;
        UINT16 PacketId = 0;
        std::vector<char> Body;
    };

    // Connect / Receive로 가짜 사건 준비
    void Connect(SessionId sessionId, Clock::time_point connectedAt = Clock::now())
    {
        const bool inserted = m_connected.insert(sessionId).second;

        if (inserted == false)
            return;

        Event event;
        event.type = EventType::Connected;
        event.Session = sessionId;
        event.OccurredAt = connectedAt;

        m_events.push_back(std::move(event));
    }

    void Receive(SessionId sessionId, UINT16 packetId, std::vector<char>body)
    {
        if (!IsConnected(sessionId))
            return;

        Event event;
        event.type = EventType::Packet;
        event.Session = sessionId;
        event.PacketId = packetId;
        event.Body = std::move(body);

        m_events.push_back(std::move(event));
    }

    bool TryPopEvent(Event& outEvent) override
    {
        if (m_events.empty())
        {
            return false;
        }

        outEvent = std::move(m_events.front());
        m_events.pop_front();

        return true;
    }

    bool IsConnected(SessionId sessionId) const override
    {
        return m_connected.find(sessionId) != m_connected.end();
    }

    bool SendPacket(SessionId sessionId, std::uint16_t packetId, const char* body, size_t bodySize) override
    {
        if (!IsConnected(sessionId))
        {
            return false;
        }

        if (bodySize > 0 && body == nullptr)
        {
            return false;
        }

        SentPacket packet;
        packet.Session = sessionId;
        packet.PacketId = packetId;

        if (bodySize > 0)
        {
            packet.Body.assign(body, body + bodySize);
        }

        Sent.push_back(std::move(packet));
        return true;
    }

    void Disconnect(SessionId sessionId) override
    {
        if (m_connected.erase(sessionId) == 0)
        {
            return;
        }

        Event event;
        event.type = EventType::Disconnected;
        event.Session = sessionId;

        m_events.push_back(std::move(event));
    }

public:
    std::vector<SentPacket> Sent;//실제 전송 대신 기록을 남김

private:
    std::unordered_set<SessionId> m_connected;//현재 연결된 세션 ID만 저장해.
    std::deque<Event> m_events;//꺼내 갈 이벤트를 순서대로 보관
};