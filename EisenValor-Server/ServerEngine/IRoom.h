#pragma once

#include "ServerEngineStruct.h"

namespace GameServerEngine {
	class GameWorldThread;
	class IRoom {
	public:
		IRoom();
		virtual ~IRoom();

	public:
		virtual void Init(const std::unordered_map<uint32, GameWorldParticipantInfo>& info) abstract;
		virtual void Update(const float dt) abstract;
		virtual void EnterSession(std::shared_ptr<Session> session) abstract;
		virtual void LeaveSession(std::shared_ptr<Session> session) abstract;
		virtual void Broadcast(std::shared_ptr<GameServerEngine::PacketBuffer> pb) abstract;

	public:
		void SetID(const uint16 id) { m_id = id; }
		uint16 GetID() const { return m_id; }
		void SetGameWorldThread(GameWorldThread* const thread) { m_thread = thread; }
		GameWorldThread* GetGameWorldThread() const { return m_thread; }
		
	private:
		uint16				m_id;
		GameWorldThread*	m_thread;
	};
}