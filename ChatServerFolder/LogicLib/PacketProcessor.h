#pragma once
#include "../SeverNetLib/ReceivedPacket.h"

namespace NServerNetLib
{
	class TcpNetwork;
}

namespace NLogicLib
{


	class PacketProcessor
	{
	public:
		PacketProcessor(NServerNetLib::TcpNetwork& network);

		void Update();
	private:
		void Process(const ReceivedPacket& packet);


	private:

		NServerNetLib::TcpNetwork& m_network;



	};

}