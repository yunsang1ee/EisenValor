// NPC.h
#pragma once
#include "GameObject.h"
#include "Vertex.h"

class NPC : public GameObject
{
public:
	// 팀 구분용
	enum class Team
	{
		ALLY,
		ENEMY
	};

	// 유닛 타입 구분용
	enum class NPC_TYPE : uint8
	{
		NONE,
		GENERAL,
		SOLDIER,
		ARCHER,
		MEDIC,
		BATTLE_RAM,
		BOSS,

		END
	};

	NPC() = default;
	virtual ~NPC() = default;

	virtual void Initialize(ID3D12Device* device) override;
	virtual void Update(float deltaTime) override;
	virtual void Render(ID3D12GraphicsCommandList* cmdList, DirectX::XMMATRIX view, DirectX::XMMATRIX projection)
		override;

	virtual ObjectType GetObjectType() const override { return ObjectType::NPC; }

	void			BuildAccelerationStructure(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList);
	ID3D12Resource* GetBottomLevelAS() const { return m_bottomLevelAS.Get(); }

	const std::vector<Vertex>& GetVertices() const { return m_vertices; }
	const std::vector<UINT>&   GetIndices() const { return m_indices; }
	ID3D12Resource*			   GetVertexBuffer() const { return m_vertexBuffer.Get(); }
	ID3D12Resource*			   GetIndexBuffer() const { return m_indexBuffer.Get(); }
	DirectX::XMMATRIX		   GetTransform() const;

	void SetTarget(std::shared_ptr<GameObject> target);
	void SetFollowDistance(float distance) { m_followDistance = distance; }

	// 타입 설정 및 확인
	void SetTeam(Team team) { m_team = team; }
	Team GetTeam() const { return m_team; }

	void SetUnitType(NPC_TYPE type)
	{
		m_unitType = type;
		UpdateUnitProperties(); // 타입 변경시 속성 업데이트
	}
	NPC_TYPE GetUnitType() const { return m_unitType; }

	virtual RaytracingInstanceType GetRaytracingInstanceType() const override { return RaytracingInstanceType::NPC; }

private:
	std::shared_ptr<GameObject> m_target;
	float						m_followDistance = 1.0f;
	float						m_moveSpeed = 4.5f;
	float						m_baseY = 0.0f;	   // 기준 높이
	float						m_rotation = 0.0f; // NPC 회전

	// Team and Unit
	Team	 m_team = Team::ALLY;
	NPC_TYPE m_unitType = NPC_TYPE::SOLDIER;

	// DirectX 렌더링 리소스들
	ComPtr<ID3D12Resource>	 m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	ComPtr<ID3D12Resource>	m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	ComPtr<ID3D12Resource> m_constantBuffer;
	ConstantBuffer		   m_constantBufferData;
	UINT8*				   m_pCbvDataBegin = nullptr;

	ComPtr<ID3D12Resource> m_bottomLevelAS;
	ComPtr<ID3D12Resource> m_scratchBuffer;

	std::vector<Vertex> m_vertices;
	std::vector<UINT>	m_indices;

private:
	void			  UpdateUnitProperties();
	DirectX::XMFLOAT4 GetTeamColor() const;
	DirectX::XMFLOAT3 GetUnitScale() const;
};