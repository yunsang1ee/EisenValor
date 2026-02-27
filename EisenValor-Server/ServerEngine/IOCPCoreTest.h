#pragma once

#include "IOCoreTest.h"

namespace ServerEngine {
	class Session;

	class IOCPCoreTest : public IOCoreTest {
	public:
		IOCPCoreTest() = default;
		~IOCPCoreTest() = default;

	public:
		virtual bool Init() override final;
		virtual bool Register(std::shared_ptr<Session> session) override final;
		virtual void ProcessIO() override final;
	};
}

