#pragma once
#include "stdafxClientFramework.h"
#include <DirectXMath.h>

class Transform
{
private:
	Vec3 m_scale{1.f, 1.f, 1.f};	   // S - Scale
	Vec3 m_rotation{0.f, 0.f, 0.f};	   // R - Rotation (Euler angles)
	Vec3 m_translation{0.f, 0.f, 0.f}; // T - Translation
	
	// 캐싱된 월드 매트릭스와 더티 플래그
	mutable bool			  m_isDirty = true;
	mutable DirectX::XMMATRIX m_worldMatrix;

public:
	// Scale
	const Vec3& GetScale() const { return m_scale; }
	void		SetScale(const Vec3& scale)
	{
		m_scale = scale;
		MarkDirty();
	}
	void SetScale(float x, float y, float z)
	{
		m_scale = {x, y, z};
		MarkDirty();
	}
	void SetUniformScale(float scale)
	{
		m_scale = {scale, scale, scale};
		MarkDirty();
	}

	// Rotation
	const Vec3& GetRotation() const { return m_rotation; }
	void		SetRotation(const Vec3& rotation)
	{
		m_rotation = rotation;
		MarkDirty();
	}
	void SetRotation(float x, float y, float z)
	{
		m_rotation = {x, y, z};
		MarkDirty();
	}

	// Translation
	const Vec3& GetTranslation() const { return m_translation; }
	const Vec3& GetPosition() const { return m_translation; } // 편의 함수
	void		SetTranslation(const Vec3& translation)
	{
		m_translation = translation;
		MarkDirty();
	}
	void SetPosition(const Vec3& position)
	{
		m_translation = position;
		MarkDirty();
	}
	void SetPosition(float x, float y, float z)
	{
		m_translation = {x, y, z};
		MarkDirty();
	}

	// 월드 매트릭스 (SRT 순서로 계산)
	const DirectX::XMMATRIX& GetWorldMatrix() const
	{
		if (m_isDirty)
		{
			UpdateWorldMatrix();
			m_isDirty = false;
		}
		return m_worldMatrix;
	}

private:
	void MarkDirty() { m_isDirty = true; }

	void UpdateWorldMatrix() const
	{
		// SRT 순서: Scale -> Rotation -> Translation
		DirectX::XMMATRIX scaleMatrix = DirectX::XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);
		DirectX::XMMATRIX rotationMatrix =
			DirectX::XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
		DirectX::XMMATRIX translationMatrix =
			DirectX::XMMatrixTranslation(m_translation.x, m_translation.y, m_translation.z);

		m_worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
	}
};
