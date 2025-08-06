#pragma once

class IDxDebugGlobal : public IGlobal
{
public:
    virtual void EnableDebug() = 0;
    virtual void SetupDebugMessages(ID3D12Device* device) = 0;
};

class DxDebugGlobal : public GlobalMakerBase<DxDebugGlobal, IDxDebugGlobal>
{
public:
    void EnableDebug() override;

    void SetupDebugMessages(ID3D12Device* device) override;
};