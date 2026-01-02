#pragma once
#include "DxCommon.h"

class DxResource
{
public:
	DxResource() = default;
	virtual ~DxResource();

	DxResource(const DxResource&) = delete;
	DxResource& operator=(const DxResource&) = delete;

	DxResource(DxResource&& other) noexcept;
	DxResource& operator=(DxResource&& other) noexcept;

	// Getters
	ID3D12Resource*			  GetResource() const { return m_resource.Get(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress(uint64_t offset = 0) const;
	D3D12_RESOURCE_STATES	  GetCurrentState() const { return m_currentState; }
	uint64_t				  GetSizeInBytes() const { return m_sizeInBytes; }
	bool					  IsValid() const { return m_resource != nullptr; }
	std::string_view		  GetName() const { return m_name; }

	void SetState(D3D12_RESOURCE_STATES newState) { m_currentState = newState; }
	void SetName(std::string_view name);

	EQueueType GetLastUsedQueue() const { return m_lastUsedQueue; }
	void	   SetLastUsedQueue(EQueueType queue) { m_lastUsedQueue = queue; }

protected:
	void InitializeResource(
		ID3D12Device*				 device,
		const D3D12_HEAP_PROPERTIES& heapProps,
		D3D12_HEAP_FLAGS			 heapFlags,
		const D3D12_RESOURCE_DESC&	 resourceDesc,
		D3D12_RESOURCE_STATES		 initialState,
		const D3D12_CLEAR_VALUE*	 clearValue = nullptr
	);
	void ReleaseResource();

	ComPtr<ID3D12Resource> m_resource;
	EQueueType			   m_lastUsedQueue = EQueueType::Graphics;
	D3D12_RESOURCE_STATES  m_currentState = D3D12_RESOURCE_STATE_COMMON;
	uint64_t			   m_sizeInBytes = 0;
	std::string			   m_name;
};
