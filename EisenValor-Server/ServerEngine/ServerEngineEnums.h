#pragma once

enum RIO_FIGURE {
	MAX_RIO_BUFFER_SIZE = 65536,	// 64kb
	MAX_RIO_BUFFER_COUNT = 2,

	MAX_RIO_BUFFER_CAPACITY = MAX_RIO_BUFFER_SIZE * MAX_RIO_BUFFER_COUNT,	
	
	// TODO: 여기서 부턴 성능측정해서, 가장 좋은 수치로...
	MAX_SEND_RQ_SIZE_PER_SESSION = 64,	 // 세션의 최대 SEND_RQ 크기 
	MAX_RECV_RQ_SIZE_PER_SESSION = 64,	 // 세션의 최대 RECV_RQ 크기
	MAX_SESION_PER_RIO_WORKER = 50,		 // 쓰레드 당 최대 담당 세션 개수
	MAX_CQ_SIZE	 = (MAX_SEND_RQ_SIZE_PER_SESSION  + MAX_RECV_RQ_SIZE_PER_SESSION) * MAX_SESION_PER_RIO_WORKER,
	MAX_RIO_RESULT = 512,		// 한 번에 COMPLETION_QUEUE에서 가져오는 크기


	// CQ는 쓰레드 마다 갖고 있음 -> (세션의 최대 SEND_RQ 크기 + 세션의 최대 RECV_RQ 크기) * 쓰레드 당 최대 담당 세션 개수
	// CQ는 SEND/RECV 따로 만들 수 있음 -> 지금은 하나만 만들어서 같이 사용

	// SEND_LIMIT_SIZE = 600,
	// COMMIT_TIME_MS_LIMIT = 20,
	// MAX_POST_CNT = 256,
	

	RIO_WORKER_TICK = 64,	// milliseconds
};

enum class SESSION_STATE : uint8 {
	FREE,			// Session 생성 소멸
	ALLOC,			// Accept 직후 
	IN_LOBBY,		// 클라로부터 로그인 패킷 받고 접속하면 Lobby 입장
	IN_GAME_ROOM,	// Room 입장 시
	IN_GAME,		// 게임 시작 직후
};