#pragma once
#include "Singleton.h"

class DxDebugGlobal : public Singleton<DxDebugGlobal>
{
private:
	friend class Singleton<DxDebugGlobal>;

	DxDebugGlobal() = default;
	virtual ~DxDebugGlobal() = default;

public:
	void Initialize() override;

	void SetupDebugMessages(ID3D12Device* device);
};