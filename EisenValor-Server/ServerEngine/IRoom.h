#pragma once

namespace ServerEngine {
	class IRoom {
	public:
		IRoom() = default;
		virtual ~IRoom() = default;

	public:
		virtual void Init() abstract;
		virtual void Update() abstract;
	};
}