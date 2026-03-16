#pragma once

namespace ServerEngine {
	class IOCoreTest {
	protected:

	public:
		IOCoreTest();
		virtual ~IOCoreTest();
	
	public:
		virtual bool Init() abstract;
		virtual bool Register(std::shared_ptr<Session> session) abstract;
		virtual bool Deregister(std::shared_ptr<Session> session) abstract;
		virtual void ProcessIO() abstract;
	};
}


