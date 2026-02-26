#pragma once

#include "IOCoreTest.h"

namespace ServerEngine {
	class IOCPCoreTest : public IOCoreTest {
	public:
		IOCPCoreTest() = default;
		~IOCPCoreTest() = default;

	public:
		virtual void ProcessIO() override final;
	};
}

