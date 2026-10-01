#include "Room.h"
#include <algorithm>
#include <stdexcept>
namespace NLogicLib
{
    NLogicLib::Room::Room(RoomId id, std::string title, size_t capacity, NServerNetLib::SessionId creator)
        : m_id(id), m_title(title),m_capacity(capacity)
    {
        if(id == INVALID_ROOM_ID || capacity ==0 || creator==0)
        {
            throw std::invalid_argument(
                "Invalid room construction.");
        }

        m_members.push_back(creator);
    }

    NLogicLib::Room::~Room()
    {
    }

    bool NLogicLib::Room::Contains(NServerNetLib::SessionId sessionId) const
    {
        return find( m_members.begin(), m_members.end(), sessionId) != m_members.end();
    }

    bool NLogicLib::Room::RemoveMember(NServerNetLib::SessionId sessionId)
    {
        const auto it = find(m_members.begin(), m_members.end(), sessionId);

        if (it == m_members.end())
            return false;

        m_members.erase(it);

        return true;
    }
}