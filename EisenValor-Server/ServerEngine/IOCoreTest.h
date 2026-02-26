#pragma once

namespace ServerEngine {
	class IOCoreTest {
	public:
		IOCoreTest()=default;
		virtual ~IOCoreTest();
	
	public:
		virtual void ProcessIO() abstract;
	};
}


