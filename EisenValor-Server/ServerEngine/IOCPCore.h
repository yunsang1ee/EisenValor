#pragma once

#include "IOCore.h"

#ifdef _USE_IOCP

namespace ServerEngine {
	class IOCPCore : public IOCore {
	public:
		IOCPCore();
		virtual ~IOCPCore();

	public:
		virtual bool Init() override final;
		virtual bool Register(std::shared_ptr<Session> session) override final;
		virtual bool Deregister(std::shared_ptr<Session> session) override final;
		virtual void ProcessIO() override final;
	};
}

#endif