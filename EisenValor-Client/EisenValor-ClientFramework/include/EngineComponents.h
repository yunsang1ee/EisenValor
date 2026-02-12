#pragma once
#include <tuple>
#include "IComponent.h"
#include "Transform.h"
#include "RectTransformComponent.h"
#include "CameraComponent.h"
#include "MeshComponent.h"
#include "SkinnedMeshComponent.h"
#include "MovementComponent.h" 
#include "ImageUIComponent.h"
#include "ButtonUIComponent.h"
//.. Add other engine component includes here

template <typename T>
consteval bool is_valid_component_tuple()
{
	if constexpr (requires { typename std::tuple_size<T>::type; })
	{
		return []<typename... Ts>(std::tuple<Ts...>*) -> bool { return (IsValidComponent<Ts> && ...); }((T*)nullptr);
	}
	return false;
}
template <typename T>
concept ComponentTuple = is_valid_component_tuple<T>();

using EngineComponents = std::tuple<
	Transform,
	RectTransformComponent,
	CameraComponent,
	MeshComponent,
	SkinnedMeshComponent,
	MovementComponent,
	ImageUIComponent,
	ButtonUIComponent
	//.. Add other engine components here
	>;