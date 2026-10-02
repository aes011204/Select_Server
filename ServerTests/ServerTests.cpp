// ServerTests.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//
#include "FakeNetwork.h"

#include "../ChatServerFolder/LogicLib/PacketProcessor.h"
#include "../ChatServerFolder/LogicLib/RoomManager.h"
#include "../Common/PacketID.h"
#include "../Common/PacketCode.h"
#include "../Common/RoomPacketCodec.h"
#include <chrono>
#include <exception>
#include <iostream>
#include <stdexcept>


void Check(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

const FakeNetwork::SentPacket& FindLastPacket(const FakeNetwork& network, NServerNetLib::SessionId sessionId, std::uint16_t packetId)
{
    for (auto it = network.Sent.rbegin(); it != network.Sent.rend();++it)
    {
        if (it->Session == sessionId &&
            it->PacketId == packetId)
        {
            return *it;
        }
    }

    throw std::runtime_error(
        "Expected packet was not sent");
}

void QueueLogin(FakeNetwork& network, NServerNetLib::SessionId sessionId,const std::string& nickname)
{
    Protocol::LoginRequest request;
    request.Nickname = nickname;

    network.Receive( sessionId, Protocol::LOGIN_REQ, Protocol::Encode(request));
}

void TestLogin()
{
    // 준비
    FakeNetwork network;
    NLogicLib::PacketProcessor logic(network);

    network.Connect(1);

    // 행동
    QueueLogin(network, 1, "Kim");
    logic.Update();

    // 검사
    Protocol::LoginResponse response;

    Check( Protocol::Decode(FindLastPacket(network, 1, Protocol::LOGIN_RES).Body, response),
        "Could not decode login response");

    Check(response.Result == Protocol::LoginResult::Success,
        "First login should succeed");

    // 두 번째 행동 전에 이전 송신 기록을 비운다.
    network.Sent.clear();

    QueueLogin(network, 1, "Kim");
    logic.Update();

    Check(
        Protocol::Decode(
            FindLastPacket(
                network, 1, Protocol::LOGIN_RES).Body,
            response),
        "Could not decode second login response");

    Check(
        response.Result ==
        Protocol::LoginResult::AlreadyLoggedIn,
        "Second login should be rejected");
}

void TestNicknameCleanup()
{
    FakeNetwork network;
    NLogicLib::PacketProcessor logic(network);

    network.Connect(1);
    network.Connect(2);

    QueueLogin(network, 1, "Kim");
    logic.Update();

    network.Sent.clear();

    // 다른 연결이 같은 닉네임을 요청한다.
    QueueLogin(network, 2, "Kim");
    logic.Update();

    Protocol::LoginResponse response;

    Check(
        Protocol::Decode(
            FindLastPacket(
                network, 2, Protocol::LOGIN_RES).Body,
            response),
        "Could not decode duplicate login response");

    Check(
        response.Result == Protocol::LoginResult::NicknameInUse,
        "Duplicate nickname was accepted");

    // 첫 사용자가 종료한다.
    network.Disconnect(1);
    logic.Update();

    network.Sent.clear();

    // 두 번째 사용자가 같은 닉네임으로 다시 시도한다.
    QueueLogin(network, 2, "Kim");
    logic.Update();

    Check(
        Protocol::Decode(
            FindLastPacket(
                network, 2, Protocol::LOGIN_RES).Body,
            response),
        "Could not decode login after disconnect");

    Check(
        response.Result == Protocol::LoginResult::Success,
        "Nickname was not released after disconnect");
}

void TestChatBeforeLogin()
{
    FakeNetwork network;
    NLogicLib::PacketProcessor logic(network);

    network.Connect(1);

    Protocol::ChatRequest request;
    request.Message = "Hello";

    network.Receive(
        1,
        Protocol::CHAT_REQ,
        Protocol::Encode(request));

    logic.Update();

    Protocol::ChatResponse response;

    Check(
        Protocol::Decode(
            FindLastPacket(
                network, 1, Protocol::CHAT_RES).Body,
            response),
        "Could not decode chat response");

    Check(
        response.Result == Protocol::ChatResult::NotLoggedIn,
        "Chat before login should be rejected");

    for (const auto& packet : network.Sent)
    {
        Check(
            packet.PacketId != Protocol::CHAT_NTF,
            "Rejected chat must not be broadcast");
    }
}

void TestChatBroadcast()
{
    FakeNetwork network;
    NLogicLib::PacketProcessor logic(network);

    network.Connect(1);
    network.Connect(2);
    network.Connect(3);

    QueueLogin(network, 1, "Kim");
    QueueLogin(network, 2, "Lee");

    logic.Update();

    Protocol::RoomCreateRequest createRequest;
    createRequest.Title = "Test Room";

    Protocol::PacketBody body;

    Check(
        Protocol::Encode(createRequest, body),
        "Could not encode room creation");

    network.Receive(
        1,
        Protocol::ROOM_CREATE_REQ,
        body);

    logic.Update();

    Protocol::RoomActionResponse createResponse;

    Check(
        Protocol::Decode(
            FindLastPacket(
                network, 1, Protocol::ROOM_CREATE_RES).Body,
            createResponse),
        "Could not decode room creation");

    Check(
        createResponse.Result == Protocol::RoomResult::Success,
        "Room creation should succeed");

    Protocol::RoomEnterRequest enterRequest;
    enterRequest.RoomId = createResponse.RoomId;

    Check(
        Protocol::Encode(enterRequest, body),
        "Could not encode room entry");

    network.Receive(
        2,
        Protocol::ROOM_ENTER_REQ,
        body);

    logic.Update();

    Protocol::RoomActionResponse enterResponse;

    Check(
        Protocol::Decode(
            FindLastPacket(
                network, 2, Protocol::ROOM_ENTER_RES).Body,
            enterResponse),
        "Could not decode room entry");

    Check(
        enterResponse.Result == Protocol::RoomResult::Success,
        "Room entry should succeed");

    // 입장 알림과 응답을 비운 뒤 채팅 결과만 검사한다.
    network.Sent.clear();

  

    Protocol::ChatRequest request;
    request.Message = "Hello";

    network.Receive(
        1,
        Protocol::CHAT_REQ,
        Protocol::Encode(request));

    logic.Update();

    Protocol::ChatResponse response;

    Check(
        Protocol::Decode(
            FindLastPacket(
                network, 1, Protocol::CHAT_RES).Body,
            response),
        "Could not decode chat result");

    Check(
        response.Result == Protocol::ChatResult::Success,
        "Chat should succeed");

    int kimCount = 0;
    int leeCount = 0;

    for (const auto& packet : network.Sent)
    {
        if (packet.PacketId != Protocol::CHAT_NTF)
        {
            continue;
        }

        Protocol::ChatNotification notification;

        Check(
            Protocol::Decode(packet.Body, notification),
            "Could not decode chat notification");

        Check(
            notification.Nickname == "Kim",
            "Wrong sender nickname");

        Check(
            notification.Message == "Hello",
            "Wrong chat message");

        if (packet.Session == 1)
        {
            ++kimCount;
        }
        else if (packet.Session == 2)
        {
            ++leeCount;
        }
        else
        {
            Check(false, "Chat sent to unexpected session");
        }
    }

    Check(kimCount == 1, "Sender should receive one notification");
    Check(leeCount == 1, "Other user should receive one notification");
}

void TestUnsupportedPacketIds()
{
    FakeNetwork network;
    NLogicLib::PacketProcessor logic(network);

    network.Connect(1);
    network.Connect(2);
    network.Connect(3);

    // 배열 범위 안이지만 등록되지 않은 ID
    network.Receive(1, 200, {});

    // 256칸 테이블 범위 밖
    network.Receive(2, 500, {});

    logic.Update();

    Check(
        !network.IsConnected(1),
        "Unregistered ID should disconnect its sender");

    Check(
        !network.IsConnected(2),
        "Out-of-range ID should disconnect its sender");

    Check(
        network.IsConnected(3),
        "Unrelated connection should remain active");
}

void TestLoginTimeout()
{
    FakeNetwork network;

    NLogicLib::PacketProcessor logic(
        network,
        std::chrono::seconds{ 5 });

    const auto oldTime =
        FakeNetwork::Clock::now() -
        std::chrono::seconds{ 20 };

    network.Connect(1, oldTime);

    logic.Update();

    Check(
        !network.IsConnected(1),
        "Expired connection should be disconnected");

    // 종료 이벤트에 대한 로직 정리까지 실행한다.
    logic.Update();
}

void TestLateLoginRejected()
{
    FakeNetwork network;

    NLogicLib::PacketProcessor logic(
        network,
        std::chrono::seconds{ 5 });

    const auto oldTime =
        FakeNetwork::Clock::now() -
        std::chrono::seconds{ 20 };

    network.Connect(1, oldTime);
    QueueLogin(network, 1, "Kim");

    logic.Update();

    Check(
        !network.IsConnected(1),
        "Late login should be disconnected");

    Check(
        network.Sent.empty(),
        "Expired login should not produce a success response");
}

bool RunTest(const char* name, void (*test)())
{
    try
    {
        test();

        std::cout << "[PASS] " << name << '\n';
        return true;
    }
    catch (const std::exception& error)
    {
        std::cerr
            << "[FAIL] " << name
            << ": " << error.what()
            << '\n';

        return false;
    }
}
void TestRoomLifecycle()
{
    NLogicLib::UserManager users;

    NLogicLib::RoomConfig config;
    config.MaxRooms = 1;
    config.Capacity = 2;

    NLogicLib::RoomManager rooms(users, config);

    Check(
        users.Login(1, "Kim") == Protocol::LoginResult::Success,
        "Kim login failed");

    Check(
        users.Login(2, "Lee") == Protocol::LoginResult::Success,
        "Lee login failed");

    const NLogicLib::User* kim = users.Find(1);

    Check(kim != nullptr, "Kim should exist");

    Check(
        kim->GetState() == NLogicLib::UserState::Lobby,
        "New user should be in lobby");

    Check(
        kim->GetRoomId() == NLogicLib::INVALID_ROOM_ID,
        "Lobby user should have no room");

    NLogicLib::RoomId roomId = NLogicLib::INVALID_ROOM_ID;

    const auto created =
        rooms.CreateRoom(1, "First Room", roomId);

    Check(
        created == NLogicLib::RoomResult::Success,
        "Room creation should succeed");

    const NLogicLib::Room* room = rooms.Find(roomId);

    Check(room != nullptr, "Created room should exist");

    Check(
        room->Contains(1),
        "Creator should be a room member");

    Check(
        kim->GetState() == NLogicLib::UserState::InRoom,
        "Creator should be in room");

    Check(
        kim->GetRoomId() == roomId,
        "User room ID should match");

    NLogicLib::RoomId anotherId = 0;

    Check(
        rooms.CreateRoom(1, "Another Room", anotherId) ==
        NLogicLib::RoomResult::AlreadyInRoom,
        "User already in room should not create another");

    Check(
        rooms.CreateRoom(2, "Lee Room", anotherId) ==
        NLogicLib::RoomResult::RoomLimitReached,
        "Room count limit should be enforced");

    Check(
        users.Find(2)->GetState() == NLogicLib::UserState::Lobby,
        "Failed creation must not change user state");

    Check(
        rooms.LeaveRoom(1) == NLogicLib::RoomResult::Success,
        "Leaving room should succeed");

    Check(
        kim->GetState() == NLogicLib::UserState::Lobby,
        "User should return to lobby");

    Check(
        kim->GetRoomId() == NLogicLib::INVALID_ROOM_ID,
        "Room ID should be cleared");

    Check(
        rooms.Find(roomId) == nullptr,
        "Empty room should be deleted");

    Check(
        rooms.GetRoomCount() == 0,
        "Room count should return to zero");

    // room 포인터는 방 삭제 이후 다시 사용하지 않는다.

    Check(
        rooms.CreateRoom(2, "Lee Room", anotherId) ==
        NLogicLib::RoomResult::Success,
        "Room creation should succeed after capacity is freed");

    Check(
        anotherId != roomId,
        "New room should receive a new ID");
}
int main()
{
    int failed = 0;

    if (!RunTest("Login", TestLogin))
        ++failed;

    if (!RunTest("Nickname cleanup", TestNicknameCleanup))
        ++failed;

    if (!RunTest("Chat before login", TestChatBeforeLogin))
        ++failed;

    if (!RunTest("Chat broadcast", TestChatBroadcast))
        ++failed;

    if (!RunTest("Unsupported packet IDs", TestUnsupportedPacketIds))
        ++failed;

    if (!RunTest("Login timeout", TestLoginTimeout))
        ++failed;

    if (!RunTest("Late login", TestLateLoginRejected))
        ++failed;

    // 방 관련
    if (!RunTest("Room lifecycle", TestRoomLifecycle))
        ++failed;


    std::cout
        << "Failed tests: "
        << failed
        << '\n';

    return failed == 0 ? 0 : 1;
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
