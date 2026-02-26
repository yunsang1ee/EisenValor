#pragma once

namespace ServerEngine {
	class IRoom {
	public:
		IRoom() = default;
		virtual ~IRoom() = default;

	public:
		virtual void Update() abstract;
	};
}

