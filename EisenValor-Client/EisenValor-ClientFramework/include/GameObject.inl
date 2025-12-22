#pragma once
#include "GameObject.h"
#include "Scene.h"
#include "ComponentStorage.h"

template <IsValidComponent T>
inline T* GameObject::GetComponent() const
{
	HandleOf<T> compHandle = GetComponentHandle<T>();
	auto*		compStorage = m_scene->GetStorage<T>();
	if (!compStorage)
	{
		return nullptr;
	}
	return compStorage->Get(compHandle);
}