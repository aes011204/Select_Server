#pragma once

#include <string>
#include "../SeverNetLib/NetworkEvent.h"

struct User
{
	NServerNetLib::SessionId Session = 0;
	std::string Nickname;
};