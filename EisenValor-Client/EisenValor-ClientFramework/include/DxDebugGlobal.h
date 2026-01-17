#pragma once
#include "Singleton.h"

class DxDebugGlobal : public Singleton<DxDebugGlobal>
{
private:
	friend class Singleton<DxDebugGlobal>;

	DxDebugGlobal() = default;
	~DxDebugGlobal() override = default;

public:
	void Initialize() override;

	void SetupDebugMessages(ID3D12Device* device);

	void PrintDebugMessages();
	void SetBreakOnSeverity(bool breakOnError = true, bool breakOnWarning = false);

private:
	ComPtr<ID3D12InfoQueue> m_infoQueue;
};