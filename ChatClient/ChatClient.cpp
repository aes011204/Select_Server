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
#include "../Common/RoomPacketCodec.h"
#include "ClientGameView.h"
#include "../Common/GamePacketCodec.h"
#include <sstream>


using namespace NChatClient;

// 함수 선언
bool SendEncodedBody(NChatClient::TcpClient& client, UINT16 packetId, const Protocol::PacketBody& body);
bool TryParseRoomId(const std::string& text, UINT32& out);
//bool ProcessCommand(NChatClient::TcpClient& client, const std::string& command);
bool ProcessCommand(TcpClient& client,const std::string& command,ClientGameView& game);
bool ProcessChatNotification(const NChatClient::ClientPacket& packet);
//bool ProcessResponse(const NChatClient::ClientPacket& packet);
bool ProcessResponse(const ClientPacket& packet,ClientGameView& game);
void CheckPacketCodec();
const char* RoomResultText(Protocol::RoomResult result);


bool SendEncodedBody(NChatClient::TcpClient& client,UINT16 packetId,const Protocol::PacketBody& body)
{
	const std::string bytes(body.begin(), body.end());

	return client.SendPacket(packetId, bytes);
}

bool TryParseRoomId(
	const std::string& text,
	UINT32& out)
{
	if (text.empty())
	{
		return false;
	}

	std::uint32_t value = 0;

	const auto maxValue =
		(std::numeric_limits<std::uint32_t>::max)();

	for (unsigned char ch : text)
	{
		if (ch < '0' || ch > '9')
		{
			return false;
		}

		const std::uint32_t digit = ch - '0';

		if (value > (maxValue - digit) / 10)
		{
			return false;
		}

		value = value * 10 + digit;
	}

	out = value;
	return true;
}

bool ProcessCommand(TcpClient& client, const std::string& command, ClientGameView& game)
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
		std::cout << "create <title>\n";
		std::cout << "rooms [afterRoomId]\n";
		std::cout << "enter <roomId>\n";
		std::cout << "leave\n";
		std::cout << "ready on\n";
		std::cout << "ready off\n";
		std::cout << "start\n";

		return true;
	}

	if (command == "board")
	{
		game.Print();
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

	if (command == "create" ||
		command.rfind("create ", 0) == 0)
	{
		Protocol::RoomCreateRequest request;

		request.Title =
			command == "create"
			? std::string{}
		: command.substr(7);

		Protocol::PacketBody body;

		if (!Protocol::Encode(request, body))
		{
			std::cout << "Room title is too long to encode.\n";
			return true;
		}

		return SendEncodedBody(
			client,
			Protocol::ROOM_CREATE_REQ,
			body);
	}

	if (command == "rooms" ||
		command.rfind("rooms ", 0) == 0)
	{
		Protocol::RoomListRequest request;

		if (command != "rooms" &&
			!TryParseRoomId(command.substr(6), request.AfterRoomId))
		{
			std::cout << "Usage: rooms [afterRoomId]\n";
			return true;
		}

		Protocol::PacketBody body;

		if (!Protocol::Encode(request, body))
		{
			std::cerr << "Could not encode room list request.\n";
			return false;
		}

		return SendEncodedBody(
			client,
			Protocol::ROOM_LIST_REQ,
			body);
	}

	if (command == "enter" ||
		command.rfind("enter ", 0) == 0)
	{
		Protocol::RoomEnterRequest request;

		if (command == "enter" ||
			!TryParseRoomId(command.substr(6), request.RoomId) ||
			request.RoomId == 0)
		{
			std::cout << "Usage: enter <roomId>\n";
			return true;
		}

		Protocol::PacketBody body;

		if (!Protocol::Encode(request, body))
		{
			std::cerr << "Could not encode room enter request.\n";
			return false;
		}

		return SendEncodedBody(
			client,
			Protocol::ROOM_ENTER_REQ,
			body);
	}

	if (command == "leave")
	{
		Protocol::RoomLeaveRequest request;
		Protocol::PacketBody body;

		if (!Protocol::Encode(request, body))
		{
			std::cerr << "Could not encode leave request.\n";
			return false;
		}

		return SendEncodedBody(
			client,
			Protocol::ROOM_LEAVE_REQ,
			body);
	}

	if (command == "ready on" || command == "ready off")
	{
		Protocol::RoomReadyRequest request;
		request.Ready = command == "ready on";

		Protocol::PacketBody body;

		if (!Protocol::Encode(request, body))
		{
			std::cerr << "Could not encode ready request.\n";
			return false;
		}

		return SendEncodedBody(
			client,
			Protocol::ROOM_READY_REQ,
			body);
	}

	if (command == "start")
	{
		Protocol::RoomStartRequest request;
		Protocol::PacketBody body;

		if (!Protocol::Encode(request, body))
		{
			std::cerr << "Could not encode start request.\n";
			return false;
		}

		return SendEncodedBody(
			client,
			Protocol::ROOM_START_REQ,
			body);
	}

	if (command == "move" ||
		command.rfind("move ", 0) == 0)
	{
		if (game.RoomId == 0 ||
			!game.RoomPlaying ||
			game.State != Protocol::GameState::Playing)
		{
			std::cout << "Start a game first.\n";
			return true;
		}

		std::istringstream input(command);

		std::string verb;
		std::string extra;

		int x = 0;
		int y = 0;

		if (!(input >> verb >> x >> y) ||
			(input >> extra) ||
			x < 0 || x >= 15 ||
			y < 0 || y >= 15)
		{
			std::cout << "Usage: move <x:0-14> <y:0-14>\n";
			return true;
		}

		Protocol::GameMoveRequest request;
		request.RoomId = game.RoomId;
		request.X = static_cast<std::uint8_t>(x);
		request.Y = static_cast<std::uint8_t>(y);

		Protocol::PacketBody body;

		if (!Protocol::Encode(request, body))
		{
			std::cerr << "Could not encode move request.\n";
			return false;
		}

		return SendEncodedBody(
			client,
			Protocol::GAME_MOVE_REQ,
			body);
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

bool ProcessResponse(const ClientPacket& packet, ClientGameView& game)
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

		case Protocol::ChatResult::NotInRoom:
			std::cout << "\n[Chat error] Enter a room first.\n";
			return true;

		case Protocol::ChatResult::StateMismatch:
			std::cout << "\n[Chat error] Server room state mismatch.\n";
			return true;
		default:
			std::cerr << "Unknown chat result.\n";
			return false;
		}
	}

	case Protocol::CHAT_NTF:
		return ProcessChatNotification(packet);

	case Protocol::ROOM_LIST_RES:
	{
		Protocol::RoomListResponse response;

		if (!Protocol::Decode(packet.Body, response))
		{
			std::cerr << "Invalid room list response.\n";
			return false;
		}

		if (response.Result != Protocol::RoomResult::Success)
		{
			std::cout
				<< "[Rooms] "
				<< RoomResultText(response.Result)
				<< '\n';

			return true;
		}

		if (response.Rooms.empty())
		{
			std::cout << "[Rooms] No rooms on this page.\n";
		}

		for (const auto& room : response.Rooms)
		{
			std::cout
				<< "[Room " << room.RoomId << "] "
				<< room.Title
				<< " ("
				<< static_cast<int>(room.UserCount)
				<< "/"
				<< static_cast<int>(room.Capacity)
				<< ")\n";
		}

		if (response.HasMore)
		{
			std::cout
				<< "Next page: rooms "
				<< response.Rooms.back().RoomId
				<< '\n';
		}

		return true;
	}

	case Protocol::ROOM_CREATE_RES:
	case Protocol::ROOM_ENTER_RES:
	case Protocol::ROOM_LEAVE_RES:
	case Protocol::ROOM_READY_RES:
	case Protocol::ROOM_START_RES:
	{
		Protocol::RoomActionResponse response;

		if (!Protocol::Decode(packet.Body, response))
		{
			std::cerr << "Invalid room action response.\n";
			return false;
		}

		if (packet.PacketId == Protocol::ROOM_LEAVE_RES &&
			response.Result == Protocol::RoomResult::Success)
		{
			game.Reset();
		}

		const char* action = "Room";

		switch (packet.PacketId)
		{
		case Protocol::ROOM_CREATE_RES:
			action = "Create";
			break;

		case Protocol::ROOM_ENTER_RES:
			action = "Enter";
			break;

		case Protocol::ROOM_LEAVE_RES:
			action = "Leave";
			break;

		case Protocol::ROOM_READY_RES:
			action = "Ready";
			break;

		case Protocol::ROOM_START_RES:
			action = "Start";
			break;
		}

		std::cout
			<< "[" << action << "] "
			<< RoomResultText(response.Result);

		if (response.Result == Protocol::RoomResult::Success)
		{
			std::cout << ", room ID: " << response.RoomId;
		}

		std::cout << '\n';
		return true;
	
	}

	case Protocol::ROOM_MEMBER_NTF:
	{
		Protocol::RoomMemberNotification notification;

		if (!Protocol::Decode(packet.Body, notification))
		{
			std::cerr << "Invalid room member notification.\n";
			return false;
		}

		const char* action = "";

		switch (notification.Change)
		{
		case Protocol::RoomMemberChange::Joined:
			action = "joined";
			break;

		case Protocol::RoomMemberChange::Left:
			action = "left";
			break;

		case Protocol::RoomMemberChange::Disconnected:
			action = "disconnected";
			break;
		}

		std::cout
			<< "\n[Room " << notification.RoomId << "] "
			<< notification.Nickname
			<< " " << action
			<< ".\n";

		return true;
	}
	case Protocol::ROOM_STATE_NTF:
	{
		Protocol::RoomStateNotification notification;

		if (!Protocol::Decode(packet.Body, notification))
		{
			std::cerr << "Invalid room state notification.\n";
			return false;
		}


		const bool nowPlaying =
			notification.Phase == Protocol::RoomPhase::Playing;

		if (!nowPlaying)
		{
			game.Reset(notification.RoomId);
		}
		else if (game.RoomId != notification.RoomId ||
			!game.RoomPlaying)
		{
			game.Begin(notification.RoomId);
			game.Print();
		}


		const bool playing =
			notification.Phase == Protocol::RoomPhase::Playing;

		std::cout
			<< "\n[Room " << notification.RoomId << "] "
			<< (playing ? "Playing" : "Waiting")
			<< '\n';

		std::cout
			<< "Host: "
			<< notification.HostNickname
			<< '\n';

		for (const auto& player : notification.Players)
		{
			const bool isHost =
				player.Nickname == notification.HostNickname;

			std::cout << "- " << player.Nickname;

			if (isHost)
			{
				std::cout << " [Host]";
			}
			else if (!playing)
			{
				std::cout
					<< (player.Ready
						? " [Ready]"
						: " [Not ready]");
			}

			std::cout << '\n';
		}

		return true;
	}
	case Protocol::GAME_MOVE_RES:
	{
		Protocol::GameMoveResponse response;

		if (!Protocol::Decode(packet.Body, response))
		{
			std::cerr << "Invalid move response.\n";
			return false;
		}

		if (response.Result != Protocol::GameMoveResult::Success)
		{
			std::cout
				<< "[Move] "
				<< MoveResultText(response.Result)
				<< '\n';
		}

		// 성공 시 보드는 알림 패킷에서 변경한다.
		return true;
	}
	case Protocol::GAME_MOVE_NTF:
	{
		Protocol::GameMoveNotification notification;

		if (!Protocol::Decode(packet.Body, notification))
		{
			std::cerr << "Invalid move notification.\n";
			return false;
		}

		if (!game.Apply(notification))
		{
			std::cerr << "Client board is out of sync.\n";
			return false;
		}

		std::cout
			<< "\n[Move] "
			<< (notification.PlacedStone == Protocol::GameStone::Black
				? "Black"
				: "White")
			<< " at ("
			<< static_cast<int>(notification.X)
			<< ", "
			<< static_cast<int>(notification.Y)
			<< ")\n";

		game.Print();
		return true;
	}
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

const char* RoomResultText(Protocol::RoomResult result)
{
	using R = Protocol::RoomResult;

	switch (result)
	{
	case R::Success:          return "Success";
	case R::UserNotFound:     return "Login first";
	case R::AlreadyInRoom:    return "Already in a room";
	case R::NotInRoom:        return "Not in a room";
	case R::InvalidTitle:     return "Invalid room title";
	case R::RoomLimitReached: return "Room limit reached";
	case R::RoomIdExhausted:   return "Room ID exhausted";
	case R::StateMismatch:    return "Server state mismatch";
	case R::RoomNotFound:     return "Room not found";
	case R::RoomFull:         return "Room is full";
	case R::InvalidRequest:   return "Invalid request";
	case R::NotHost:		  return "Only the host can start";
	case R::HostCannotReady:  return "Host uses the start command";
	case R::NotEnoughPlayers: return "Two players are required";
	case R::NotAllReady:		return "The other player is not ready";
	case R::GameAlreadyStarted:	return "Game already started";
	case R::GameInProgress:		return "Game is in progress";
	}

	return "Unknown result";
}

const char* MoveResultText(Protocol::GameMoveResult result)
{
	using R = Protocol::GameMoveResult;

	switch (result)
	{
	case R::Success:        return "Success";
	case R::NotLoggedIn:    return "Login first";
	case R::NotInRoom:      return "Enter a room first";
	case R::WrongRoom:      return "Room does not match";
	case R::GameNotRunning: return "Game is not running";
	case R::OutOfBounds:    return "Position is outside the board";
	case R::NotYourTurn:    return "Not your turn";
	case R::Occupied:       return "Position is already occupied";
	case R::StateMismatch:  return "Server state mismatch";
	}

	return "Unknown result";
}

int main()
{
	CheckPacketCodec();

	ClientGameView game;

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

	bool running;
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

				running = ProcessCommand(client, input, game);
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
			if (!ProcessResponse(packet, game))
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
