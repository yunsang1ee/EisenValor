#pragma once

namespace ServerEngine {
	class Session;

	class IOCoreTest {
	public:
		IOCoreTest();
		virtual ~IOCoreTest();
	
	public:
		virtual bool Init() abstract;
		virtual bool Register(std::shared_ptr<Session> session) abstract;
		virtual void ProcessIO() abstract;
	};
}


