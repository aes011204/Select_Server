// ChatClient.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "TcpClient.h"
#include <conio.h>
#include <iostream>
#include <iostream>
#include <string>
#include <stdexcept>
#include "../Common/PacketID.h"
#include "../Common/PacketUtils.h"
#include "../Common/PacketCode.h"
using namespace NChatClient;
bool SendEncodedBody(NChatClient::TcpClient& client,UINT16 packetId,const Protocol::PacketBody& body)
{
	const std::string bytes(body.begin(), body.end());

	return client.SendPacket(packetId, bytes);
}

bool ProcessCommand(
	TcpClient& client,
	const std::string& command)
{
	if (command == "quit")
	{
		return false;
	}

	if (command == "help")
	{
		std::cout << "login <nickname>\n";
		std::cout << "echo <message>\n";
		std::cout << "quit\n";
		return true;
	}

	if (command == "login" ||
		command.rfind("login ", 0) == 0)
	{
		const std::string nickname =
			command == "login"
			? std::string{}
		: command.substr(6);

		Protocol::LoginRequest request;
		request.Nickname = nickname;

		const auto body = Protocol::Encode(request);

		if (!SendEncodedBody(client, Protocol::LOGIN_REQ, body))
		{
			std::cerr << "Could not queue login request.\n";
			return false;
		}

		return true;
	}

	if (command == "echo" ||
		command.rfind("echo ", 0) == 0)
	{
		const std::string message =
			command == "echo"
			? std::string{}
		: command.substr(5);

		if (!client.SendPacket(
			Protocol::ECHO_REQ,
			message))
		{
			std::cerr << "Could not queue echo request.\n";
			return false;
		}

		return true;
	}

	if (command == "chat" ||
		command.rfind("chat ", 0) == 0)
	{
		const std::string message =
			command == "chat"
			? std::string{}
		: command.substr(5);

		Protocol::ChatRequest request;
		request.Message = message;

		const auto body = Protocol::Encode(request);

		if (!SendEncodedBody(client, Protocol::CHAT_REQ, body))
		{
			std::cerr << "Could not queue chat request.\n";
			return false;
		}

		return true;
	}

	if (!command.empty())
	{
		std::cout << "Unknown command. Type help.\n";
	}

	return true;
}

bool ProcessChatNotification(const ClientPacket& packet)
{
	Protocol::ChatNotification notification;

	if (!Protocol::Decode(packet.Body, notification))
	{
		std::cerr << "Invalid chat notification.\n";
		return false;
	}

	std::cout
		<< "\n[Chat] "
		<< notification.Nickname
		<< ": "
		<< notification.Message
		<< '\n';

	return true;
}

bool ProcessResponse(const ClientPacket& packet)
{
	switch (packet.PacketId)
	{
	case Protocol::LOGIN_RES:
	{
		Protocol::LoginResponse response;

		if (!Protocol::Decode(packet.Body, response))
		{
			std::cerr << "Invalid login response.\n";
			return false;
		}

		const auto result = response.Result;

		// 이 아래의 기존 switch (result)는 그대로 유지

		switch (result)
		{
		case Protocol::LoginResult::Success:
			std::cout << "[Login] Success\n";
			break;

		case Protocol::LoginResult::InvalidNickname:
			std::cout
				<< "[Login] Use 1-16 letters, digits or underscore.\n";
			break;

		case Protocol::LoginResult::NicknameInUse:
			std::cout << "[Login] Nickname is already in use.\n";
			break;

		case Protocol::LoginResult::AlreadyLoggedIn:
			std::cout << "[Login] This connection is already logged in.\n";
			break;

		default:
			std::cerr << "Unknown login result.\n";
			return false;
		}

		return true;
	}

	case Protocol::ECHO_RES:
	{
		const std::string text(
			packet.Body.begin(),
			packet.Body.end()
		);

		std::cout << "[Echo] " << text << '\n';
		return true;
	}
	case Protocol::CHAT_RES:
	{
		Protocol::ChatResponse response;

		if (!Protocol::Decode(packet.Body, response))
		{
			std::cerr << "Invalid chat response.\n";
			return false;
		}

		const auto result = response.Result;

		switch (result)
		{
		case Protocol::ChatResult::Success:
			// 실제 채팅 내용은 CHAT_NTF에서 한 번만 출력
			return true;

		case Protocol::ChatResult::NotLoggedIn:
			std::cout << "\n[Chat error] Login first.\n";
			return true;

		case Protocol::ChatResult::InvalidMessage:
			std::cout
				<< "\n[Chat error] Use 1-256 UTF-8 bytes, "
				<< "with no control characters or only spaces.\n";
			return true;

		default:
			std::cerr << "Unknown chat result.\n";
			return false;
		}
	}

	case Protocol::CHAT_NTF:
		return ProcessChatNotification(packet);
	default:
		std::cerr << "Unexpected packet ID: "
			<< packet.PacketId << '\n';

		return false;
	}
}

void CheckPacketCodec()
{
	Protocol::ChatNotification original;
	original.Nickname = "Kim";
	original.Message = "Hi";

	Protocol::PacketBody encoded;

	if (!Protocol::Encode(original, encoded))
	{
		throw std::runtime_error("Encode failed");
	}

	const Protocol::PacketBody expected{
		3, 'K', 'i', 'm',
		0, 2, 'H', 'i'
	};

	if (encoded != expected)
	{
		throw std::runtime_error("Wire format mismatch");
	}

	Protocol::ChatNotification decoded;

	if (!Protocol::Decode(encoded, decoded) ||
		decoded.Nickname != "Kim" ||
		decoded.Message != "Hi")
	{
		throw std::runtime_error("Decode failed");
	}

	auto truncated = encoded;
	truncated.pop_back();

	if (Protocol::Decode(truncated, decoded))
	{
		throw std::runtime_error("Truncated packet accepted");
	}

	auto extra = encoded;
	extra.push_back('X');

	if (Protocol::Decode(extra, decoded))
	{
		throw std::runtime_error("Extra byte accepted");
	}

	Protocol::LoginResponse response;

	if (Protocol::Decode(Protocol::PacketBody{ 99 }, response))
	{
		throw std::runtime_error("Unknown result accepted");
	}

	std::cout << "[Codec] Checks passed.\n";
}

int main()
{
	CheckPacketCodec();

	TcpClient client;

	if (!client.Connect("127.0.0.1", 32452))
	{
		std::cerr << "Connection failed.\n";
		std::cout << "Press Enter to exit.\n";
		std::cin.get();

		return 1;
	}

	std::cout << "Connected to server.\n";
	std::cout << "Commands: login <nickname>, echo <message>, quit\n";
	std::cout << "> ";

	bool running = true;
	std::string input;


	while (running)
	{
		while (_kbhit())
		{
			const int key = _getch();
			// 방향키·기능키의 확장 입력은 무시
			if (key == 0 || key == 224)
			{
				_getch();
				continue;
			}

			if (key == '\r')
			{
				std::cout << '\n';

				running = ProcessCommand(client, input);
				input.clear();

				if (!running)
				{
					break;
				}

				std::cout << "> ";
			}
			else if (key == '\b')
			{
				if (!input.empty())
				{
					input.pop_back();
					std::cout << "\b \b";
				}
			}
			else if (key >= 32 && key <= 126)
			{
				if (input.size() < 1024)
				{
					input.push_back(static_cast<char>(key));
					std::cout << static_cast<char>(key);
				}
			}
		}

		if (!running)
		{
			break;
		}

		if (!client.Run())
		{
			break;
		}

		ClientPacket packet;

		while (client.TryPopPacket(packet))
		{
			if (!ProcessResponse(packet))
			{
				running = false;
				break;
			}
		}

		

	}

	client.Disconnect();

	std::cout << "Client stopped.\n";

	return 0;
}

// 프로그램 실행: <Ctrl+F5> 또는 [디버그] > [디버깅하지 않고 시작] 메뉴
// 프로그램 디버그: <F5> 키 또는 [디버그] > [디버깅 시작] 메뉴

// 시작을 위한 팁: 
//   1. [솔루션 탐색기] 창을 사용하여 파일을 추가/관리합니다.
//   2. [팀 탐색기] 창을 사용하여 소스 제어에 연결합니다.
//   3. [출력] 창을 사용하여 빌드 출력 및 기타 메시지를 확인합니다.
//   4. [오류 목록] 창을 사용하여 오류를 봅니다.
//   5. [프로젝트] > [새 항목 추가]로 이동하여 새 코드 파일을 만들거나, [프로젝트] > [기존 항목 추가]로 이동하여 기존 코드 파일을 프로젝트에 추가합니다.
//   6. 나중에 이 프로젝트를 다시 열려면 [파일] > [열기] > [프로젝트]로 이동하고 .sln 파일을 선택합니다.
