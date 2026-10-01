#pragma once

#include "NetworkEvent.h"
namespace NServerNetLib
{
	class INetwork
	{
	public:
		INetwork() = default;
		virtual ~INetwork()= default;
	
	public:
		virtual bool TryPopEvent(NetworkEvent& outEvent) = 0;
		virtual bool IsConnected(SessionId sessionId) const = 0;
		virtual bool SendPacket(SessionId sessionId,UINT16 packetId, const char* body,size_t bodysize) = 0;
		virtual void Disconnect(SessionId sessionId) = 0;
	
	};
}