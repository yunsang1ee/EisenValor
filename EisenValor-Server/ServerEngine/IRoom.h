#pragma once

namespace ServerEngine {
	class IRoom {
	public:
		IRoom();
		virtual ~IRoom();

	public:
		virtual void Init() abstract;
		virtual void Update(const float dt) abstract;
		virtual void EnterSession(std::shared_ptr<Session> session) abstract;
		virtual void LeaveSession(std::shared_ptr<Session> session) abstract;
		virtual void Broadcast(std::shared_ptr<ServerEngine::PacketBuffer> pb) abstract;
		
	};
}