#pragma once
class DxCommandContext
{
public:
	DxCommandContext(ID3D12Device* device,
		D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
	~DxCommandContext();

	void Reset();
	void Close();
	void Execute(class IDxGraphicsCommandQueueGlobal& queue);

	ID3D12GraphicsCommandList*	CommandList()	{ return m_commandList.Get(); }
	ID3D12CommandAllocator*		Allocator()		{ return m_allocator.Get(); }
	D3D12_COMMAND_LIST_TYPE		Type() const	{ return m_type; }

private:
	ComPtr<ID3D12CommandAllocator>		m_allocator;
	ComPtr<ID3D12GraphicsCommandList>	m_commandList;
	D3D12_COMMAND_LIST_TYPE				m_type;
};

