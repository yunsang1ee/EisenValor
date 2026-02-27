#pragma once

namespace ServerEngine {
	class IRoom {
	public:
		IRoom();
		virtual ~IRoom();

	public:
		virtual void Init() abstract;
		virtual void Update() abstract;
		virtual void EnterSession(std::shared_ptr<Session> session) abstract;
		
	};
}