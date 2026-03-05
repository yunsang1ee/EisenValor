#pragma once

enum class IO_CONTEXT_TYPE : uint8 {
	ACCEPT,
	RECV,
	SEND,
};

enum class SESSION_STATE : uint8 {
	FREE,					// Session 생성 소멸
	ACCEPTED,				// Accept 직후 
	IN_GAME_LOBBY,			// 클라로부터 로그인 패킷 받고 접속하면 Lobby 입장
	IN_GAME_ROOM,			// Room 입장 시
	TRANSFERRING,		
	IN_GAME_WORLD,			// 게임 시작 직후
};