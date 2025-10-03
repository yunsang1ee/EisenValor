// NPC.h
#pragma once
#include "GameObject.h"
#include "Vertex.h"

class NPC : public GameObject
{
public:
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

	void SetTarget(std::shared_ptr<GameObject> target);
	void SetFollowDistance(float distance) { m_followDistance = distance; }

	void SetUnitType(NPC_TYPE type)
	{
		m_unitType = type;
	}
	virtual void SetTeamColor() override;
	NPC_TYPE GetUnitType() const { return m_unitType; }

private:
    std::shared_ptr<GameObject> m_target;
    float m_followDistance = 1.0f;
    float m_moveSpeed = 4.5f;
    float m_baseY = 0.0f;        // 기준 높이
    // float m_rotation = 0.0f;     // NPC 회전
    // 
	NPC_TYPE m_unitType = NPC_TYPE::SOLDIER; 

    // DirectX 렌더링 리소스들
    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

    ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

    ComPtr<ID3D12Resource> m_constantBuffer;
    ConstantBuffer m_constantBufferData;
    UINT8* m_pCbvDataBegin = nullptr;

private:
	void			  UpdateUnitProperties();
	DirectX::XMFLOAT3 GetUnitScale() const;
};