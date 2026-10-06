// ServerTests.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//
#include "FakeNetwork.h"

#include "../ChatServerFolder/LogicLib/PacketProcessor.h"
#include "../ChatServerFolder/LogicLib/RoomManager.h"
#include "../ChatServerFolder/LogicLib/OmokGame.h"
#include "../Common/PacketID.h"
#include "../Common/PacketCode.h"
#include "../Common/RoomPacketCodec.h"
#include "../Common/GamePacketCodec.h"
#include <chrono>
#include <exception>
#include <iostream>
#include <stdexcept>
using Stone = Protocol::GameStone;
using GameStatus = Protocol::GameState;


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
void TestReadyAndStartRules()
{
    NLogicLib::UserManager users;
    NLogicLib::RoomManager rooms(users);

    Check(
        users.Login(1, "Kim") == Protocol::LoginResult::Success,
        "Kim login failed");

    Check(
        users.Login(2, "Lee") == Protocol::LoginResult::Success,
        "Lee login failed");

    NLogicLib::RoomId roomId = 0;

    Check(
        rooms.CreateRoom(1, "Game Room", roomId) ==
        NLogicLib::RoomResult::Success,
        "Room creation failed");

    Check(
        rooms.StartGame(1) ==
        NLogicLib::RoomResult::NotEnoughPlayers,
        "One player must not start");

    Check(
        rooms.EnterRoom(2, roomId) ==
        NLogicLib::RoomResult::Success,
        "Room entry failed");

    Check(
        rooms.StartGame(2) ==
        NLogicLib::RoomResult::NotHost,
        "Guest must not start");

    Check(
        rooms.SetReady(1, true) ==
        NLogicLib::RoomResult::HostCannotReady,
        "Host must not use ready");

    Check(
        rooms.StartGame(1) ==
        NLogicLib::RoomResult::NotAllReady,
        "Unready guest must prevent start");

    Check(
        rooms.SetReady(2, true) ==
        NLogicLib::RoomResult::Success,
        "Guest ready failed");

    Check(
        rooms.StartGame(1) ==
        NLogicLib::RoomResult::Success,
        "Ready room should start");

    const auto* room = rooms.Find(roomId);

    Check(
        room != nullptr &&
        room->GetPhase() == Protocol::RoomPhase::Playing,
        "Room should be playing");


    Check(
        room->GetGame().GetStatus() ==
        Protocol::GameState::Playing,
        "Starting room should start OmokGame");

    Check(
        room->GetGame().GetNextTurn() ==
        Protocol::GameStone::Black,
        "New game should begin with black");

    Check(
        room->GetPlayerStone(1) == Protocol::GameStone::Black,
        "Host should play black");

    Check(
        room->GetPlayerStone(2) == Protocol::GameStone::White,
        "Guest should play white");


    Check(
        !room->IsReady(2),
        "Ready state should be cleared after start");

    Check(
        rooms.SetReady(2, false) ==
        NLogicLib::RoomResult::GameInProgress,
        "Ready must not change during game");


    NLogicLib::AcceptedMove accepted;

    Check(
        rooms.PlaceStone(2, roomId, 7, 7, accepted) ==
        Protocol::GameMoveResult::NotYourTurn,
        "White must not move first");

    Check(
        rooms.PlaceStone(1, roomId, 7, 7, accepted) ==
        Protocol::GameMoveResult::Success,
        "Black move should succeed");

    Check(
        accepted.PlacedStone == Protocol::GameStone::Black &&
        accepted.NextTurn == Protocol::GameStone::White &&
        accepted.MoveCount == 1,
        "Accepted move data is incorrect");

    Check(
        rooms.PlaceStone(2, roomId, 7, 7, accepted) ==
        Protocol::GameMoveResult::Occupied,
        "Occupied position must be rejected");

    Check(
        rooms.PlaceStone(2, roomId, 15, 0, accepted) ==
        Protocol::GameMoveResult::OutOfBounds,
        "Server must reject invalid coordinates");

    Check(
        rooms.PlaceStone(2, roomId + 100, 7, 8, accepted) ==
        Protocol::GameMoveResult::WrongRoom,
        "Request must match the actual room");

    Check(
        rooms.PlaceStone(2, roomId, 7, 8, accepted) ==
        Protocol::GameMoveResult::Success,
        "White should still be able to move after rejections");



    Check(
        rooms.LeaveRoom(1) ==
        NLogicLib::RoomResult::Success,
        "Host leave failed");

    room = rooms.Find(roomId);

    Check(room != nullptr, "Guest room should remain");

    Check(
        room->GetHostSession() == 2,
        "Remaining player should become host");

    Check(
        room->GetPhase() == Protocol::RoomPhase::Waiting,
        "Room should return to waiting");



    Check(
        room->GetGame().GetStatus() ==
        Protocol::GameState::NotStarted,
        "Leaving should reset the game");

    Check(
        room->GetPlayerStone(2) == Protocol::GameStone::Empty,
        "Leaving should clear player color assignments");

}
void TestGameStartAndValidation()
{
    using namespace NLogicLib;

    OmokGame game;

    Check(
        game.TryPlaceStone(Stone::Black, 7, 7) ==
        MoveResult::GameNotRunning,
        "Game must start before placing stones");

    game.Start();

    Check(
        game.GetStatus() == GameStatus::Playing,
        "Game should be playing");

    Check(
        game.GetNextTurn() == Stone::Black,
        "Black should move first");

    Check(
        game.GetMoveCount() == 0,
        "New game should have no moves");

    Check(
        game.TryPlaceStone(Stone::White, 7, 7) ==
        MoveResult::NotYourTurn,
        "White must not move first");

    Check(
        game.TryPlaceStone(Stone::Black, -1, 7) ==
        MoveResult::OutOfBounds,
        "Negative coordinate must be rejected");

    Check(
        game.TryPlaceStone(Stone::Black, 15, 7) ==
        MoveResult::OutOfBounds,
        "Coordinate 15 must be rejected");

    Check(
        game.TryPlaceStone(Stone::Empty, 7, 7) ==
        MoveResult::InvalidStone,
        "Empty is not a playable stone");

    Check(
        game.GetMoveCount() == 0 &&
        game.GetNextTurn() == Stone::Black &&
        game.GetStone(7, 7) == Stone::Empty,
        "Rejected moves must not change the game");

    Check(
        game.TryPlaceStone(Stone::Black, 7, 7) ==
        MoveResult::Success,
        "Black move should succeed");

    Check(
        game.GetStone(7, 7) == Stone::Black &&
        game.GetMoveCount() == 1 &&
        game.GetNextTurn() == Stone::White,
        "Accepted move should update board and turn");

    Check(
        game.TryPlaceStone(Stone::White, 7, 7) ==
        MoveResult::Occupied,
        "Occupied position must be rejected");

    Check(
        game.GetNextTurn() == Stone::White &&
        game.GetMoveCount() == 1,
        "Rejected occupied move must preserve turn");
}
void CheckBlackWinLine(
    int startX,
    int startY,
    int dx,
    int dy)
{
    using namespace NLogicLib;

    OmokGame game;
    game.Start();

    for (int i = 0; i < 5; ++i)
    {
        Check(
            game.TryPlaceStone(
                Stone::Black,
                startX + dx * i,
                startY + dy * i) == MoveResult::Success,
            "Black line move failed");

        if (i < 4)
        {
            // 테스트하는 흑의 연결과 겹치지 않는 위치.
            Check(
                game.TryPlaceStone(
                    Stone::White,
                    i * 2,
                    14) == MoveResult::Success,
                "White filler move failed");
        }
    }

    Check(
        game.GetStatus() == GameStatus::BlackWon,
        "Five connected black stones should win");

    Check(
        game.GetNextTurn() == Stone::Empty,
        "Finished game should have no next turn");

    Check(
        game.TryPlaceStone(Stone::White, 14, 13) ==
        MoveResult::GameNotRunning,
        "Finished game must reject further moves");
}
void TestWinningDirections()
{
    CheckBlackWinLine(0, 0, 1, 0);  // 가로
    CheckBlackWinLine(0, 0, 0, 1);  // 세로
    CheckBlackWinLine(0, 0, 1, 1);  // 오른쪽 아래 대각선
    CheckBlackWinLine(0, 4, 1, -1); // 오른쪽 위 대각선
}

void TestOverlineAndBothDirections()
{
    using namespace NLogicLib;

    OmokGame game;
    game.Start();

    const int blackXs[] = { 0, 1, 2, 4, 5 };

    for (int i = 0; i < 5; ++i)
    {
        Check(
            game.TryPlaceStone(
                Stone::Black, blackXs[i], 7) ==
            MoveResult::Success,
            "Black setup move failed");

        Check(
            game.TryPlaceStone(
                Stone::White, i * 2, 14) ==
            MoveResult::Success,
            "White setup move failed");
    }

    Check(
        game.GetStatus() == GameStatus::Playing,
        "Separated stones must not win");

    Check(
        game.TryPlaceStone(Stone::Black, 3, 7) ==
        MoveResult::Success,
        "Connecting move should succeed");

    Check(
        game.GetStatus() == GameStatus::BlackWon,
        "Six connected stones should win in freestyle rules");
}
void TestWhiteWin()
{
    using namespace NLogicLib;

    OmokGame game;
    game.Start();

    for (int i = 0; i < 5; ++i)
    {
        Check(
            game.TryPlaceStone(
                Stone::Black, i * 2, 14) ==
            MoveResult::Success,
            "Black filler move failed");

        Check(
            game.TryPlaceStone(
                Stone::White, i, 0) ==
            MoveResult::Success,
            "White line move failed");
    }

    Check(
        game.GetStatus() == GameStatus::WhiteWon,
        "Five connected white stones should win");
}

void TestGameReset()
{
    using namespace NLogicLib;

    OmokGame game;
    game.Start();

    Check(
        game.TryPlaceStone(Stone::Black, 7, 7) ==
        MoveResult::Success,
        "Setup move failed");

    game.Reset();

    Check(
        game.GetStatus() == GameStatus::NotStarted,
        "Reset should stop the game");

    Check(
        game.GetNextTurn() == Stone::Empty,
        "Reset should clear the turn");

    Check(
        game.GetMoveCount() == 0,
        "Reset should clear move count");

    for (const auto& row : game.GetBoard())
    {
        for (auto stone : row)
        {
            Check(
                stone == Stone::Empty,
                "Reset should clear every board cell");
        }
    }

    game.Start();

    Check(
        game.GetStatus() == GameStatus::Playing &&
        game.GetNextTurn() == Stone::Black,
        "Restart should begin with black");
}
void TestTurnTimeout()
{
    using namespace NLogicLib;

    UserManager users;

    RoomConfig config;
    config.TurnTime = std::chrono::seconds{ 5 };

    RoomManager rooms(users, config);

    Check(
        users.Login(1, "Kim") == Protocol::LoginResult::Success,
        "Kim login failed");

    Check(
        users.Login(2, "Lee") == Protocol::LoginResult::Success,
        "Lee login failed");

    RoomId roomId = 0;

    Check(
        rooms.CreateRoom(1, "Timer Room", roomId) ==
        RoomResult::Success,
        "Room creation failed");

    Check(
        rooms.EnterRoom(2, roomId) == RoomResult::Success,
        "Room entry failed");

    Check(
        rooms.SetReady(2, true) == RoomResult::Success,
        "Ready failed");

    const GameClock::time_point t0{};

    Check(
        rooms.StartGame(1, t0) == RoomResult::Success,
        "Game start failed");

    AcceptedMove accepted;

    // 백의 잘못된 요청은 시간을 연장하지 않아야 한다.
    Check(
        rooms.PlaceStone(
            2,
            roomId,
            7,
            7,
            accepted,
            t0 + std::chrono::seconds{ 4 }) ==
        Protocol::GameMoveResult::NotYourTurn,
        "White must not move first");

    rooms.UpdateTimeouts(
        t0 + std::chrono::seconds{ 5 });

    FinishedGame finished;

    Check(
        rooms.TryPopFinishedGame(finished),
        "Timeout should create a game result");

    Check(
        finished.Status == GameStatus::WhiteWon,
        "Black timeout should give white the win");

    Check(
        finished.Reason == Protocol::GameEndReason::TurnTimeout,
        "Wrong end reason");

    Check(
        rooms.Find(roomId)->GetPhase() ==
        Protocol::RoomPhase::Waiting,
        "Room should return to waiting");

    // 같은 종료를 다시 생성하면 안 된다.
    rooms.UpdateTimeouts(
        t0 + std::chrono::seconds{ 100 });

    Check(
        !rooms.TryPopFinishedGame(finished),
        "Game must finish only once");
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

    if (!RunTest("Ready and start rules", TestReadyAndStartRules))
    {
        ++failed;
    }

    if (!RunTest("Game validation", TestGameStartAndValidation))
        ++failed;

    if (!RunTest("Winning directions", TestWinningDirections))
        ++failed;

    if (!RunTest("Overline", TestOverlineAndBothDirections))
        ++failed;

    if (!RunTest("White win", TestWhiteWin))
        ++failed;

    if (!RunTest("Game reset", TestGameReset))
        ++failed;

    if (!RunTest("Turn timeout", TestTurnTimeout))
    {
        ++failed;
    }

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
