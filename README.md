# Select_Server

C++과 Windows Winsock의 `select`를 사용한 TCP 채팅·오목 서버 프로젝트입니다.
콘솔 클라이언트로 접속해 채팅, 방 생성·입장, 준비 및 오목 대전을 진행할 수 있습니다.

## 구성

- `ChatClient`: 콘솔 클라이언트
- `ChatServerFolder/ConsoleHost`: 서버 실행 및 설정
- `ChatServerFolder/SeverNetLib`: TCP 연결과 데이터 송수신
- `ChatServerFolder/LogicLib`: 유저·방 관리, 패킷 처리 및 오목 로직
- `Common`: 공통 패킷 정의와 인코딩·디코딩
- `ServerTests`: 서버 로직 테스트

## 실행

1. Windows에서 Visual Studio 2022의 **C++를 사용한 데스크톱 개발** 워크로드를 설치합니다.
2. `SelectChatServer.sln`을 열고 `Debug | x64`로 솔루션을 빌드합니다.
3. `ConsoleHost`를 실행한 뒤 `ChatClient`를 실행합니다. 기본 접속 주소는 `127.0.0.1:32452`입니다.
4. 클라이언트에서 `help`를 입력하면 사용 가능한 명령을 확인할 수 있습니다. 대전 시 클라이언트를 두 개 실행합니다.

테스트는 `ServerTests` 프로젝트를 빌드한 뒤 실행합니다.
