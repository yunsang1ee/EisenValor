#pragma once

#include "IOCoreTest.h"

namespace ServerEngine {
	class SessionTest;

	class IOCPCoreTest : public IOCoreTest {
	public:
		IOCPCoreTest();
		virtual ~IOCPCoreTest();

	public:
		virtual bool Init() override final;
		virtual bool Register(std::shared_ptr<Session> session) override final;
		virtual void ProcessIO() override final;
	};
}

