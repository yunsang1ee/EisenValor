#pragma once
#include "stdafxClientFramework.h"
#include "DxCommon.h"
#include "Transform.h"
#include "IComponent.h"        
#include "ComponentTypes.h"   
#include <array>            
#include <vector>              

class GameObject : public std::enable_shared_from_this<GameObject>
{
public:
	GameObject() = default;
	virtual ~GameObject() = default;

	// 모든 게임 오브젝트가 구현해야 하는 순수 가상 함수들
	virtual void Initialize(ID3D12Device* device) = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void Render(ID3D12GraphicsCommandList* cmdList, DirectX::XMMATRIX view, DirectX::XMMATRIX projection) = 0;

	// 공통 기능들
	virtual void SetPosition(const Vec3& pos)
	{
		m_transform.SetPosition(pos); // Transform 업데이트
	}
	virtual Vec3 GetPosition() const
	{
		return m_transform.GetPosition(); // Transform에서 반환
	}
	virtual void SetRotation(const Vec3& rot)
	{
		m_transform.SetRotation(rot); // Transform
	}
	virtual Vec3 GetRotation() const
	{
		return m_transform.GetRotation(); // Transform에서 반환
	}
	virtual void SetVelocity(const Vec3& velocity) { m_velocity = velocity; }
	virtual Vec3 GetVelocity() const { return m_velocity; }
	virtual void SetAccelration(const Vec3& acceleration) { m_acceleration = acceleration; }
	virtual Vec3 GetAcceleration() const { return m_acceleration; }

	virtual FB_ENUMS::GAME_OBJECT_TYPE GetObjectType() const = 0;
	
	// Transform 접근자
	Transform&		 GetTransform() { return m_transform; }
	const Transform& GetTransform() const { return m_transform; }

	// 핵심 컴포넌트 접근 함수들
	template <typename T>
	T* GetCoreComponent(CoreComponentType type)
	{
		auto& component = m_coreComponents[static_cast<size_t>(type)];
		return static_cast<T*>(component.get());
	}

	template <typename T>
	const T* GetCoreComponent(CoreComponentType type) const
	{
		const auto& component = m_coreComponents[static_cast<size_t>(type)];
		return static_cast<const T*>(component.get());
	}

	template <typename T>
	void AddCoreComponent(CoreComponentType type, std::unique_ptr<T> component)
	{
		component->SetGameObject(shared_from_this());
		m_coreComponents[static_cast<size_t>(type)] = std::move(component);
	}

	// 기타 컴포넌트 접근 함수들
	template <typename T>
	T* GetComponent()
	{
		for (auto& comp : m_additionalComponents)
		{
			if (auto* result = dynamic_cast<T*>(comp.get()))
				return result;
		}
		return nullptr;
	}

	template <typename T>
	const T* GetComponent() const
	{
		for (const auto& comp : m_additionalComponents)
		{
			if (const auto* result = dynamic_cast<const T*>(comp.get()))
				return result;
		}
		return nullptr;
	}

	template <typename T>
	void AddComponent(std::unique_ptr<T> component)
	{
		component->SetGameObject(shared_from_this());
		m_additionalComponents.push_back(std::move(component));
	}

	// 컴포넌트 업데이트 함수
	void UpdateComponents(float deltaTime)
	{
		// 핵심 컴포넌트 업데이트
		for (auto& comp : m_coreComponents)
		{
			if (comp && comp->IsActive())
				comp->Update(deltaTime);
		}

		// 기타 컴포넌트 업데이트
		for (auto& comp : m_additionalComponents)
		{
			if (comp->IsActive())
				comp->Update(deltaTime);
		}
	}

public:
	void Handle_SC_MOVE(
		const Vec3& pos, const Vec3& rot, const Vec3& velocity, const Vec3& accel, const uint64 timeStamp
	);
	// 타입 설정 및 확인
	void SetTeam(const FB_ENUMS::TEAM_TYPE team);
	FB_ENUMS::TEAM_TYPE GetTeam() const { return m_team; }

	virtual void SetTeamColor() {}

	 // HP 관련 함수들
	void   SetCurrentHP(uint32 hp) { m_currentHP = hp; }
	void   SetMaxHP(uint32 hp) { m_maxHP = hp; }
	uint32 GetCurrentHP() const { return m_currentHP; }
	uint32 GetMaxHP() const { return m_maxHP; }
	float  GetHPRatio() const { return static_cast<float>(m_currentHP) / static_cast<float>(m_maxHP); }

public:
	uint32 m_id;
	bool   alive{true};

protected:
	Vec3 m_velocity{0.f, 0.f, 0.f};
	Vec3 m_acceleration{0.f, 0.f, 0.f};
	
	// Transform 컴포넌트
	Transform m_transform;

	// 핵심 컴포넌트들 (array)
	std::array<std::unique_ptr<IComponent>, CORE_COMPONENT_COUNT> m_coreComponents;

	// 기타 컴포넌트들 (vector)
	std::vector<std::unique_ptr<IComponent>> m_additionalComponents;

	FB_ENUMS::TEAM_TYPE m_team = FB_ENUMS::TEAM_TYPE_BLUE;
	Vec4 m_teamColor;

	// HP 관련 변수 추가 (HP 수정용)
	uint32 m_currentHP = 100;
	uint32 m_maxHP = 100;

public:
	Vec3	lastServerPosition;
	Vec3	lastServerVelocity;	
	Vec3	lastServerAcceleration;
	Vec3	lastServerRotation;
	uint64	lastServerTimestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();

public:
	bool keyup{false};

	public:
	Vec3 SmoothLerp(const Vec3& curPos, const Vec3& destPos, const float lerpFactor);
};
