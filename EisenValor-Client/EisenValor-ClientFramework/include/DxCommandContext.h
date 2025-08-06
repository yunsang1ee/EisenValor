#pragma once

enum class DxCommandContextState
{
	Idle,
	Recording,
	Closed,
	Executing
};

class DxCommandContext
{
public:
	DxCommandContext(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
	~DxCommandContext();

	void Reset();
	void Close();
	void Execute(class IDxGraphicsCommandQueueGlobal& queue);
	void MarkAsCompleted();

	ID3D12GraphicsCommandList* CommandList() { return m_commandList.Get(); }
	ID3D12CommandAllocator*	   Allocator() { return m_allocator.Get(); }
	D3D12_COMMAND_LIST_TYPE	   Type() const { return m_type; }

	DxCommandContextState GetState() const { return m_state; }
	bool				  IsRecording() const { return m_state == DxCommandContextState::Recording; }
	bool				  IsClosed() const { return m_state == DxCommandContextState::Closed; }
	bool				  IsExecuting() const { return m_state == DxCommandContextState::Executing; }
	bool				  IsIdle() const { return m_state == DxCommandContextState::Idle; }

private:
	ComPtr<ID3D12CommandAllocator>	  m_allocator;
	ComPtr<ID3D12GraphicsCommandList> m_commandList;
	D3D12_COMMAND_LIST_TYPE			  m_type;
	DxCommandContextState			  m_state = DxCommandContextState::Idle;
};
