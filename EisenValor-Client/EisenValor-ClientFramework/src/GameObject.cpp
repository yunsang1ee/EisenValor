#include "stdafxClientFramework.h"
#include "GameObject.h"
#include "Scene.h"
#include "Transform.h"
#include "ComponentStorage.h"

GameObject::GameObject(Scene* scene, HandleOf<Transform> trHd, std::string name)
	: m_scene(scene), m_transform(trHd), m_name(std::move(name)), m_handle(Handle::Invalid()), m_serverID(kInvalidServerID)
{
}

inline Transform& GameObject::GetTransform()
{
	auto* trStorage = m_scene->GetStorage<Transform>();
	assert(trStorage && "Transform storage not registered!");
	return *trStorage->Get(m_transform);
}

const Transform& GameObject::GetTransform() const
{
	auto* trStorage = m_scene->GetStorage<Transform>();
	assert(trStorage && "Transform storage not registered!");
	return *trStorage->Get(m_transform);
}

DX::XMFLOAT4X4 GameObject::GetWorldMatrix()
{
	return GetTransform().GetWorldMatrix();
}
