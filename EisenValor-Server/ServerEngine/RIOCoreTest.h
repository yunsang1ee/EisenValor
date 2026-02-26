#pragma once

#include "IOCPCoreTest.h"

namespace ServerEngine {
	class RIOCoreTest : public IOCoreTest {
	public:
		RIOCoreTest() = default;
		virtual ~RIOCoreTest() = default;

	public:
		virtual void ProcessIO() override final;

	};
}


