#pragma once

namespace ServerEngine {
	class IOCore {
	public:
		IOCore()=default;
		virtual ~IOCore()=default;

	public:
		virtual bool Init(const SessionFactoryFunc func) abstract;
		virtual bool StartAccept() abstract;
		virtual void Run() abstract;
		virtual void Shutdown() abstract;

	protected:
		void DistributeReservedTask();
		void FlushTaskQueue();
	};
}